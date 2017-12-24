#include "face.hpp"
#include "logger.hpp"
#include "../security/hmac-key.hpp"

#include "../ndn-cpp/c/encoding/tlv/tlv.h"
#include "../ndn-cpp/lite/encoding/tlv-0_2-wire-format-lite.hpp"

#define FACE_DBG(...) DBG(Face, __VA_ARGS__)

namespace ndn {

Face::Face(Transport& transport)
  : m_transport(transport)
  , m_interestCb(nullptr)
  , m_interestCbArg(nullptr)
  , m_dataCb(nullptr)
  , m_dataCbArg(nullptr)
  , m_outArr(m_outBuf, NDNFACE_OUTBUF_SIZE, nullptr)
  , m_sigBuf(nullptr)
  , m_signingKey(nullptr)
  , m_ownsSigningKey(false)
  , m_thisData(nullptr)
{
}

Face::~Face()
{
  if (m_ownsSigningKey) {
    delete m_signingKey;
  }
  if (m_sigBuf != nullptr) {
    free(m_sigBuf);
  }
}

void
Face::onInterest(InterestCallback cb, void* cbarg)
{
  m_interestCb = cb;
  m_interestCbArg = cbarg;
}

void
Face::onData(DataCallback cb, void* cbarg)
{
  m_dataCb = cb;
  m_dataCbArg = cbarg;
}

void
Face::setSigningKey(const PrivateKey& pvtkey)
{
  bool needNewSigBuf = m_signingKey == nullptr || pvtkey.getMaxSigLength() > m_signingKey->getMaxSigLength();
  if (needNewSigBuf && m_sigBuf != nullptr) {
    free(m_sigBuf);
  }

  if (m_ownsSigningKey) {
    delete m_signingKey;
  }
  m_signingKey = &pvtkey;

  if (needNewSigBuf) {
    m_sigBuf = reinterpret_cast<uint8_t*>(malloc(pvtkey.getMaxSigLength()));
  }
}

void
Face::setHmacKey(const uint8_t* key, size_t keySize)
{
  auto hmacKey = new HmacKey(key, keySize);
  this->setSigningKey(*hmacKey);
}

void
Face::loop()
{
  uint64_t endpointId;

  int packetLimit = NDNFACE_RECEIVE_MAX;
  size_t pktSize;
  while (--packetLimit >= 0 && (pktSize = m_transport.receive(m_inBuf, NDNFACE_INBUF_SIZE, &endpointId)) > 0) {
    this->processPacket(pktSize, endpointId);
    yield();
  }
}

void
Face::processPacket(size_t len, uint64_t endpointId)
{
  switch (m_inBuf[0]) {
    case ndn_Tlv_Interest:
      this->processInterest(len, endpointId);
      break;
    case ndn_Tlv_Data:
      this->processData(len, endpointId);
      break;
    default:
      FACE_DBG(F("received unknown TLV-TYPE: 0x") << _HEX(m_inBuf[0]));
      break;
  }
}

void
Face::processInterest(size_t len, uint64_t endpointId)
{
  if (m_interestCb == nullptr) {
    FACE_DBG(F("received Interest, no handler"));
    return;
  }

  ndn_NameComponent nameComps[NDNFACE_NAMECOMPS_MAX];
  ndn_ExcludeEntry excludeEntries[NDNFACE_EXCLUDE_MAX];
  ndn_NameComponent keyNameComps[NDNFACE_KEYNAMECOMPS_MAX];
  InterestLite interest(nameComps, NDNFACE_NAMECOMPS_MAX, excludeEntries, NDNFACE_EXCLUDE_MAX, keyNameComps, NDNFACE_KEYNAMECOMPS_MAX);
  ndn_Error error = Tlv0_2WireFormatLite::decodeInterest(interest, m_inBuf, len, &m_signedBegin, &m_signedEnd);
  if (error) {
    FACE_DBG(F("received Interest decoding error: ") << _DEC(error));
    return;
  }

  m_interestCb(m_interestCbArg, interest, endpointId);
}

void
Face::processData(size_t len, uint64_t endpointId)
{
  if (m_dataCb == nullptr) {
    FACE_DBG(F("received Data, no handler"));
    return;
  }

  ndn_NameComponent nameComps[NDNFACE_NAMECOMPS_MAX];
  ndn_NameComponent keyNameComps[NDNFACE_KEYNAMECOMPS_MAX];
  DataLite data(nameComps, NDNFACE_NAMECOMPS_MAX, keyNameComps, NDNFACE_KEYNAMECOMPS_MAX);
  m_thisData = &data;
  ndn_Error error = Tlv0_2WireFormatLite::decodeData(data, m_inBuf, len, &m_signedBegin, &m_signedEnd);
  if (error) {
    FACE_DBG(F("received Data decoding error: ") << _DEC(error));
    return;
  }

  m_dataCb(m_dataCbArg, data, endpointId);
  m_thisData = nullptr;
}

bool
Face::verifyData(const PublicKey& pubkey) const
{
  if (m_thisData == nullptr) {
    return false;
  }

  const ndn::BlobLite& signatureBits = m_thisData->getSignature().getSignature();
  if (signatureBits.isNull()) {
    return false;
  }

  return pubkey.verify(m_inBuf + m_signedBegin, m_signedEnd - m_signedBegin,
                       signatureBits.buf(), signatureBits.size());
}

void
Face::sendPacket(const uint8_t* pkt, size_t len, uint64_t endpointId)
{
  m_transport.send(pkt, len, endpointId);
}

void
Face::sendInterest(InterestLite& interest, uint64_t endpointId)
{
  size_t len;
  ndn_Error error = Tlv0_2WireFormatLite::encodeInterest(interest, &m_signedBegin, &m_signedEnd, m_outArr, &len);
  if (error) {
    FACE_DBG(F("send Interest encoding error: ") << _DEC(error));
    return;
  }

  this->sendPacket(m_outBuf, len, endpointId);
}

void
Face::sendData(DataLite& data, uint64_t endpointId)
{
  if (m_signingKey == nullptr) {
    FACE_DBG(F("cannot sign Data: signing key is unset"));
    return;
  }
  ndn_Error error = m_signingKey->setSignatureInfo(data.getSignature());
  if (error) {
    FACE_DBG(F("setSignatureInfo error: ") << _DEC(error));
    return;
  }

  size_t len;
  error = Tlv0_2WireFormatLite::encodeData(data, &m_signedBegin, &m_signedEnd, m_outArr, &len);
  if (error) {
    FACE_DBG(F("send Data encoding-1 error: ") << _DEC(error));
    return;
  }

  int sigLen = m_signingKey->sign(m_outBuf + m_signedBegin, m_signedEnd - m_signedBegin, m_sigBuf);
  if (sigLen == 0) {
    FACE_DBG(F("signing error"));
    return;
  }
  data.getSignature().setSignature(BlobLite(m_sigBuf, sigLen));
  error = Tlv0_2WireFormatLite::encodeData(data, &m_signedBegin, &m_signedEnd, m_outArr, &len);
  if (error) {
    FACE_DBG(F("send Data encoding-2 error: ") << _DEC(error));
    return;
  }

  this->sendPacket(m_outBuf, len, endpointId);
}

} // namespace ndn
