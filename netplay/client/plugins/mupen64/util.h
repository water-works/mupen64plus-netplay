#ifndef CLIENT_PLUGINS_MUPEN64_UTIL_H_
#define CLIENT_PLUGINS_MUPEN64_UTIL_H_

#include "base/netplayServiceProto.pb.h"

namespace util {

// Returns the report representation of a mupen64 requested port, as used in the 
// m64 configuration.
Port M64RequestedIntToPort(int m64_requested_port);
int PortToM64RequestedInt(Port port);

// Returns the index into the mupen64 data structures mapped to by this port. 
// Returns -1 for values not representing particular ports (PORT_ANY, UKNOWN).
int PortToM64Port(Port port);
Port M64PortToPort(int port);

}  // namespace util

#endif  // CLIENT_PLUGINS_MUPEN64_UTIL_H_
