#include "test-common.hpp"

class KeyFixture : public TestOnce
{
public:
  static std::pair<const uint8_t*, size_t>
  getInput()
  {
    static const uint8_t THE_INPUT[] {
      0x79, 0x6F, 0x75, 0x72, 0x73, 0x75, 0x6E, 0x6E, 0x79, 0x2E, 0x63, 0x6F, 0x6D};
    return {THE_INPUT, sizeof(THE_INPUT)};
  }

  int
  sign(const ndn::PrivateKey& key)
  {
    const uint8_t* input;
    size_t inputLen;
    std::tie(input, inputLen) = getInput();

    sig.resize(key.getMaxSigLength());
    int sigLen = key.sign(input, inputLen, sig.data());
    sig.resize(sigLen);
    return sigLen;
  }

  String
  getSigHex() const
  {
    return toHexString(sig.data(), sig.size());
  }

  bool
  verify(const ndn::PublicKey& key) const
  {
    const uint8_t* input;
    size_t inputLen;
    std::tie(input, inputLen) = getInput();

    return key.verify(input, inputLen, sig.data(), sig.size());
  }

  void
  modifySig()
  {
    if (sig.empty()) {
      return;
    }
    size_t pos = sig.size() / 2;
    sig[pos] += 1;
  }

  void
  teardown() override
  {
    std::vector<uint8_t> empty;
    sig.swap(empty);
  }

public:
  std::vector<uint8_t> sig;
};

testF(KeyFixture, DigestKey_basic)
{
  ndn::DigestKey key;
  assertEqual(key.getMaxSigLength(), ndn_SHA256_DIGEST_SIZE);
  assertEqual(this->sign(key), static_cast<int>(ndn_SHA256_DIGEST_SIZE));
  // echo -n 'yoursunny.com' | openssl sha256
  assertEqual(this->getSigHex(),
              F("EC41C17185C911F9F2FED2A8F6CC9E1AB4E9F06C7E62DD2E4BAEFC5B56596043"));
  assertTrue(this->verify(key));

  this->modifySig();
  assertFalse(this->verify(key));
}

testF(KeyFixture, HmacKey_basic)
{
  ndn::HmacKey key(reinterpret_cast<const uint8_t*>("secret"), 6);
  assertEqual(key.getMaxSigLength(), ndn_SHA256_DIGEST_SIZE);
  assertEqual(this->sign(key), static_cast<int>(ndn_SHA256_DIGEST_SIZE));
  // echo -n 'yoursunny.com' | openssl sha256 -hmac 'secret'
  assertEqual(this->getSigHex(),
              F("E6680A517902FF958545AF733359499BDC59FAB051FCFAC02A0F253C099DA12E"));
  assertTrue(this->verify(key));

  this->modifySig();
  assertFalse(this->verify(key));
}

const uint8_t EcKey_basic_PVT[] PROGMEM {
  0xD7, 0xCB, 0xA5, 0xF4, 0xC6, 0x89, 0x3E, 0x56, 0x2B, 0xFA, 0xD1, 0x52, 0x8A, 0x8A, 0xCD, 0x4C,
  0x29, 0x3C, 0xCA, 0xC1, 0x37, 0x7F, 0x87, 0x90, 0x9A, 0x14, 0x6E, 0xA0, 0x15, 0xD8, 0x88, 0x6E,
};
const uint8_t EcKey_basic_PUB[] PROGMEM {   0x04,
  0x35, 0xF3, 0xE6, 0x92, 0x49, 0x13, 0x58, 0x46, 0xDB, 0xDA, 0x83, 0x68, 0xEA, 0x4D, 0x89, 0xD0,
  0xA0, 0x6B, 0x6C, 0xAE, 0xD4, 0xBC, 0x70, 0x41, 0x00, 0x41, 0x71, 0xA8, 0x72, 0xAB, 0x41, 0x2C,
  0x86, 0x22, 0xB5, 0x29, 0x94, 0xE1, 0x96, 0x8C, 0x3A, 0x5E, 0xC6, 0x79, 0x49, 0x2E, 0x3F, 0xF8,
  0x86, 0x56, 0xAF, 0x15, 0xEB, 0x7B, 0x82, 0x73, 0xAA, 0xB7, 0x8F, 0x34, 0xEA, 0x42, 0x35, 0xFD,
};
testF(KeyFixture, EcKey_basic)
{
  ndn::NameWCB<1> name;
  name.append("key");
  ndn::EcPrivateKey pvt(name);
  ndn::EcPublicKey pub;

  assertLessOrEqual(this->sign(pvt), 0);
  assertTrue(pvt.import(EcKey_basic_PVT));
  assertMore(this->sign(pvt), 0);
  yield();

  assertFalse(this->verify(pub));
  assertTrue(pub.import(EcKey_basic_PUB));
  assertTrue(this->verify(pub));
  yield();

  this->modifySig();
  assertFalse(this->verify(pub));
}

const uint8_t EcKey_importCert_CERT[] PROGMEM {
  0x06, 0xfd, 0x02, 0x11, 0x07, 0x23, 0x08, 0x01, 0x41, 0x08, 0x03, 0x4b, 0x45, 0x59, 0x08, 0x08,
  0x27, 0x00, 0xee, 0x14, 0xa8, 0xb1, 0x33, 0x04, 0x08, 0x04, 0x73, 0x65, 0x6c, 0x66, 0x08, 0x09,
  0xfd, 0x00, 0x00, 0x01, 0x66, 0xbc, 0xe8, 0xb8, 0x74, 0x14, 0x09, 0x18, 0x01, 0x02, 0x19, 0x04,
  0x00, 0x36, 0xee, 0x80, 0x15, 0xfd, 0x01, 0x4f, 0x30, 0x82, 0x01, 0x4b, 0x30, 0x82, 0x01, 0x03,
  0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01, 0x30, 0x81, 0xf7, 0x02, 0x01, 0x01, 0x30,
  0x2c, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x01, 0x01, 0x02, 0x21, 0x00, 0xff, 0xff, 0xff,
  0xff, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x30, 0x5b, 0x04,
  0x20, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xfc, 0x04, 0x20, 0x5a, 0xc6, 0x35, 0xd8, 0xaa, 0x3a, 0x93, 0xe7, 0xb3, 0xeb, 0xbd, 0x55, 0x76,
  0x98, 0x86, 0xbc, 0x65, 0x1d, 0x06, 0xb0, 0xcc, 0x53, 0xb0, 0xf6, 0x3b, 0xce, 0x3c, 0x3e, 0x27,
  0xd2, 0x60, 0x4b, 0x03, 0x15, 0x00, 0xc4, 0x9d, 0x36, 0x08, 0x86, 0xe7, 0x04, 0x93, 0x6a, 0x66,
  0x78, 0xe1, 0x13, 0x9d, 0x26, 0xb7, 0x81, 0x9f, 0x7e, 0x90, 0x04, 0x41, 0x04, 0x6b, 0x17, 0xd1,
  0xf2, 0xe1, 0x2c, 0x42, 0x47, 0xf8, 0xbc, 0xe6, 0xe5, 0x63, 0xa4, 0x40, 0xf2, 0x77, 0x03, 0x7d,
  0x81, 0x2d, 0xeb, 0x33, 0xa0, 0xf4, 0xa1, 0x39, 0x45, 0xd8, 0x98, 0xc2, 0x96, 0x4f, 0xe3, 0x42,
  0xe2, 0xfe, 0x1a, 0x7f, 0x9b, 0x8e, 0xe7, 0xeb, 0x4a, 0x7c, 0x0f, 0x9e, 0x16, 0x2b, 0xce, 0x33,
  0x57, 0x6b, 0x31, 0x5e, 0xce, 0xcb, 0xb6, 0x40, 0x68, 0x37, 0xbf, 0x51, 0xf5, 0x02, 0x21, 0x00,
  0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xbc, 0xe6, 0xfa, 0xad, 0xa7, 0x17, 0x9e, 0x84, 0xf3, 0xb9, 0xca, 0xc2, 0xfc, 0x63, 0x25, 0x51,
  0x02, 0x01, 0x01, 0x03, 0x42, 0x00, 0x04, 0x66, 0x4f, 0xe3, 0xd8, 0x7b, 0x70, 0x2f, 0xf4, 0x66,
  0x19, 0xe6, 0x42, 0xbf, 0xbb, 0x50, 0xca, 0x5a, 0x42, 0xb0, 0xc5, 0x56, 0xa7, 0x6f, 0x69, 0x4c,
  0x34, 0xb6, 0x9b, 0x74, 0xdc, 0x5d, 0x6f, 0xba, 0x0b, 0x54, 0x41, 0x4f, 0xf9, 0xc6, 0xf2, 0x76,
  0xbc, 0x0e, 0x90, 0x40, 0xbd, 0x68, 0x79, 0x88, 0x9c, 0xa0, 0xdc, 0x93, 0x4d, 0x49, 0xde, 0x54,
  0x22, 0x2e, 0x5e, 0x4a, 0x27, 0xd0, 0x74, 0x16, 0x43, 0x1b, 0x01, 0x03, 0x1c, 0x14, 0x07, 0x12,
  0x08, 0x01, 0x41, 0x08, 0x03, 0x4b, 0x45, 0x59, 0x08, 0x08, 0x27, 0x00, 0xee, 0x14, 0xa8, 0xb1,
  0x33, 0x04, 0xfd, 0x00, 0xfd, 0x26, 0xfd, 0x00, 0xfe, 0x0f, 0x31, 0x39, 0x37, 0x30, 0x30, 0x31,
  0x30, 0x31, 0x54, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0xfd, 0x00, 0xff, 0x0f, 0x32, 0x30, 0x33,
  0x38, 0x31, 0x30, 0x32, 0x33, 0x54, 0x32, 0x33, 0x30, 0x31, 0x30, 0x30, 0x17, 0x47, 0x30, 0x45,
  0x02, 0x21, 0x00, 0xdd, 0xd0, 0x3f, 0x78, 0x5a, 0xf9, 0x85, 0x9f, 0xfe, 0xd8, 0x3b, 0xd0, 0x5e,
  0x10, 0x83, 0x43, 0x31, 0x94, 0x57, 0xaf, 0x0c, 0x2e, 0x42, 0xf4, 0xd7, 0x5d, 0x82, 0xd6, 0xca,
  0x89, 0xb2, 0xb2, 0x02, 0x20, 0x1c, 0x2f, 0x78, 0xb9, 0x15, 0x8d, 0x2a, 0xd9, 0xc1, 0x54, 0x20,
  0xc8, 0x89, 0xa8, 0x9c, 0x41, 0x63, 0x65, 0xdd, 0x95, 0x69, 0x09, 0xf1, 0x2a, 0x13, 0x03, 0x7b,
  0x43, 0xb2, 0xf5, 0xe2, 0xc4
};
testF(KeyFixture, EcKey_importCert)
{
  std::vector<uint8_t> cert;
  cert.resize(sizeof(EcKey_importCert_CERT));
  memcpy_P(cert.data(), EcKey_importCert_CERT, cert.size());

  ndn::DataWCB<5, 3> data;
  size_t signedBegin, signedEnd;
  ndn_Error e = ndn::Tlv0_2WireFormatLite::decodeData(
    data, cert.data(), cert.size(), &signedBegin, &signedEnd);
  assertEqual(e, NDN_ERROR_success);

  ndn::EcPublicKey key;
  assertTrue(key.importCert(data));
}

testF(KeyFixture, EcKey_generate)
{
  ndn::NameWCB<1> name;
  name.append("key");
  ndn::EcPrivateKey pvt(name);
  ndn::EcPublicKey pub;
  assertTrue(pvt.generate(pub));
  yield();

  assertMore(this->sign(pvt), 0);
  yield();
  assertTrue(this->verify(pub));
  yield();
  this->modifySig();
  assertFalse(this->verify(pub));
}
