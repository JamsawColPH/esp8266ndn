#include "ping-client.hpp"
#include "../core/logger.hpp"
#include "../ndn-cpp/c/util/crypto.h"

#define PINGCLIENT_DBG(...) DBG(PingClient, __VA_ARGS__)

namespace ndn {

PingClient::Interval::Interval(int center, int variation)
  : min(center - abs(variation))
  , max(center + abs(variation))
{
}

int
PingClient::Interval::operator()() const
{
  return random(this->min, this->max);
}

static inline int
determineTimeout(int pingTimeout, const InterestLite& interest)
{
  if (pingTimeout > 0) {
    return pingTimeout;
  }
  if (interest.getInterestLifetimeMilliseconds() < 0.0) {
    return 4000;
  }
  return static_cast<int>(interest.getInterestLifetimeMilliseconds());
}

PingClient::PingClient(Face& face, InterestLite& interest, Interval pingInterval, int pingTimeout)
  : m_face(face)
  , m_interest(interest)
  , m_pingInterval(pingInterval)
  , m_pingTimeout(determineTimeout(pingTimeout, interest))
  , m_lastProbe(millis())
  , m_isPending(false)
  , m_nextProbe(0)
  , m_evtCb(nullptr)
{
  if (m_pingInterval.min <= m_pingTimeout) {
    PINGCLIENT_DBG(F("ERROR: minimum interval should be greater than timeout"));
  }

  m_face.addHandler(this);
}

PingClient::~PingClient()
{
  m_face.removeHandler(this);
}

uint32_t
PingClient::getLastSeq() const
{
  uint64_t seq;
  if (m_interest.getName().get(-1).toSequenceNumber(seq) == NDN_ERROR_success) {
    return static_cast<uint32_t>(seq);
  }
  else {
    return 0;
  }
}

void
PingClient::loop()
{
  unsigned long now = millis();
  if (m_isPending && now - m_lastProbe > m_pingTimeout) {
    m_isPending = false;
    uint32_t seq = this->getLastSeq();
    PINGCLIENT_DBG(F("timeout seq=") << _HEX(seq));
    if (m_evtCb != nullptr) {
      m_evtCb(m_evtCbArg, Event::TIMEOUT, seq);
    }
  }

  if (now > m_nextProbe) {
    this->probe();
  }
}

bool
PingClient::processData(const DataLite& data, uint64_t endpointId)
{
  if (!m_interest.getName().match(data.getName())) {
    return false;
  }
  m_isPending = false;

  uint32_t seq = this->getLastSeq();
  PINGCLIENT_DBG(F("response seq=") << _HEX(seq) << F(" rtt=") << _DEC(millis() - m_lastProbe));
  if (m_evtCb != nullptr) {
    m_evtCb(m_evtCbArg, Event::RESPONSE, seq);
  }

  return true;
}

bool
PingClient::processNack(const NetworkNackLite& nackHeader, const InterestLite& interest, uint64_t endpointId)
{
  if (!m_interest.getName().equals(interest.getName())) {
    return false;
  }
  m_isPending = false;

  uint32_t seq = this->getLastSeq();
  PINGCLIENT_DBG(F("nack seq=") << _HEX(seq) << F(" rtt=") << _DEC(millis() - m_lastProbe));
  if (m_evtCb != nullptr) {
    m_evtCb(m_evtCbArg, Event::NACK, seq);
  }

  return true;
}

void
PingClient::probe()
{
  NameLite& name = m_interest.getName();
  uint32_t seq = this->getLastSeq();
  if (seq == 0 && !name.get(-1).isSequenceNumber()) {
    ndn_generateRandomBytes(reinterpret_cast<uint8_t*>(&seq), sizeof(seq));
  }
  else {
    --reinterpret_cast<ndn_Name&>(name).nComponents;
  }
  ++seq;
  name.appendSequenceNumber(seq, m_seqBuf, sizeof(m_seqBuf));

  PINGCLIENT_DBG(F("probe seq=") << _HEX(seq));
  m_face.sendInterest(m_interest);

  m_isPending = true;
  m_lastProbe = millis();
  m_nextProbe = m_lastProbe + m_pingInterval();

  if (m_evtCb != nullptr) {
    m_evtCb(m_evtCbArg, Event::PROBE, seq);
  }
}

} // namespace ndn