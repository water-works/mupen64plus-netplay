#ifndef HOST_UTILS_H_
#define HOST_UTILS_H_

#include <memory>
#include <string>

#include "base/netplayServiceProto.grpc.pb.h"

namespace host_utils {

// Creates a new console on the server using the specified stub. Returns true
// if all RPC calls succeeded. On SUCCESS, populates *console_id with the
// allocated console ID.
bool MakeConsole(
    const std::string& console_name, const std::string& rom_name,
    const std::shared_ptr<NetPlayServerService::StubInterface>& stub,
    MakeConsoleResponsePB::Status* status, int* console_id);

// Requests that the game start.
bool StartGame(int console_id,
               const std::shared_ptr<NetPlayServerService::StubInterface>& stub,
               StartGameResponsePB::Status* status);

}  // namespace host_utils

#endif  // HOST_UTILS_H_
