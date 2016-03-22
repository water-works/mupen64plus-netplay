#include "client/plugins/mupen64/util.h"

#include "glog/logging.h"

namespace util {

Port M64RequestedIntToPort(int m64_requested_port) {
  switch (m64_requested_port) {
    case 0: return PORT_ANY;
    case 1: return PORT_1;
    case 2: return PORT_2;
    case 3: return PORT_3;
    case 4: return PORT_4;
    default: return UNKNOWN;
  }
}

int PortToM64RequestedInt(Port port) {
  switch (port) {
    case PORT_ANY: return 0;
    case PORT_1: return 1;
    case PORT_2: return 2;
    case PORT_3: return 3;
    case PORT_4: return 4;
    case UNKNOWN: return -1;
    default: {
      LOG(ERROR) << "Logic error: Called IntToM64ReqestedPort on invalid port: "
                 << Port_Name(port) << ". Crashing.";
      exit(1);
    }
  }
}

int PortToM64Port(Port port) {
  switch (port) {
    case PORT_1: return 0;
    case PORT_2: return 1;
    case PORT_3: return 2;
    case PORT_4: return 3;
    default: return -1;
  }
}

Port M64PortToPort(int port) {
  switch (port) {
    case 0: return PORT_1;
    case 1: return PORT_2;
    case 2: return PORT_3;
    case 3: return PORT_4;
    default: return UNKNOWN;
  }
}

}   // namespace util
