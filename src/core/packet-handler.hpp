#ifndef ESP8266NDN_PACKET_HANDLER_HPP
#define ESP8266NDN_PACKET_HANDLER_HPP

#include "packet-buffer.hpp"

namespace ndn {

/** \brief Base class to receive callbacks from Face.
 */
class PacketHandler
{
public:
  virtual
  ~PacketHandler() = default;

private:
  virtual bool
  processInterest(const InterestLite& interest, uint64_t endpointId);

  virtual bool
  processData(const DataLite& data, uint64_t endpointId);

  virtual bool
  processNack(const NetworkNackLite& nackHeader, const InterestLite& interest, uint64_t endpointId);

private:
  PacketHandler* m_next = nullptr;
  int8_t m_prio = 0;

  friend class Face;
};

} // namespace ndn

#endif // ESP8266NDN_PACKET_HANDLER_HPP
