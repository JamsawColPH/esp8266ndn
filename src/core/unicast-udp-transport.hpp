#ifndef ESP8266NDN_UDP_TRANSPORT_HPP
#define ESP8266NDN_UDP_TRANSPORT_HPP

#include "transport.hpp"
#include <Udp.h>

namespace ndn {

/** \brief a Transport that communicates over UDP tunnel to a single remote NDN routerIp
 */
class UnicastUdpTransport : public Transport
{
public:
  explicit
  UnicastUdpTransport(UDP& udp);

  void
  begin(IPAddress routerIp, uint16_t routerPort, uint16_t localPort);

  void
  end();

  virtual size_t
  receive(uint8_t* buf, size_t bufSize, uint64_t* endpointId) override;

  virtual void
  send(const uint8_t* pkt, size_t len, uint64_t endpointId) override;

private:
  UDP& m_udp;
  IPAddress m_routerIp;
  uint16_t m_routerPort;
};

} // namespace ndn

#endif // ESP8266NDN_UDP_TRANSPORT_HPP