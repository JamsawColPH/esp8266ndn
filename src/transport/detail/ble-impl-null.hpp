#ifndef ESP8266NDN_BLE_IMPL_NULL_HPP
#define ESP8266NDN_BLE_IMPL_NULL_HPP

#include <WString.h>

namespace ndn {
namespace detail {

class BleClientImpl;

class BleDeviceImplClass
{
public:
  int
  init(const char* deviceName, bool enableServer, int nClients)
  {
    return __LINE__;
  }

  String
  getAddr()
  {
    return F("(no-BLE)");
  }

  int
  startScanConnect(BleClientImpl& client)
  {
    return __LINE__;
  }
};

extern BleDeviceImplClass BleDeviceImpl;

class BleServiceImpl
{
public:
  int
  begin()
  {
    return __LINE__;
  }

  int
  advertise()
  {
    return __LINE__;
  }

  size_t
  receive(uint8_t* buf, size_t bufSize)
  {
    return 0;
  }

  bool
  send(const uint8_t* pkt, size_t len)
  {
    return false;
  }
};

class BleClientImpl
{
public:
  bool
  begin()
  {
    return false;
  }

  size_t
  receive(uint8_t* buf, size_t bufSize)
  {
    return 0;
  }

  bool
  send(const uint8_t* pkt, size_t len)
  {
    return false;
  }
};

} // namespace detail
} // namespace ndn

#endif // ESP8266NDN_BLE_IMPL_NULL_HPP
