#ifndef SERVER_CONSOLE_H_
#define SERVER_CONSOLE_H_

#include "base/netplayServiceProto.pb.h"
#include "base/netplayServiceProto.grpc.pb.h"

namespace server {

class Console {
 public:
  Console(long console_id);

 private:
  const long console_id_;
};

}  // namespace server

#endif  // SERVER_CONSOLE_H_
