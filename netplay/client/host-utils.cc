#include "client/host-utils.h"

#include <iostream>

#include "glog/logging.h"
#include "grpc++/client_context.h"

namespace host_utils {

bool MakeConsole(
    const std::string& console_title, const std::string& rom_name,
    const std::shared_ptr<NetPlayServerService::StubInterface>& stub,
    MakeConsoleResponsePB::Status* status, int* console_id) {
  MakeConsoleRequestPB request;

  request.set_console_title(console_title);
  request.set_rom_name(rom_name);

  MakeConsoleResponsePB response;
  grpc::ClientContext context;
  context.set_deadline(std::chrono::system_clock::now() +
                       std::chrono::milliseconds(5000));
  VLOG(3) << "Creating a console:\n" << request.DebugString();
  if (!stub->MakeConsole(&context, request, &response).ok()) {
    return false;
  }
  VLOG(3) << "Received console creation response:\n" << response.DebugString();

  *status = response.status();
  if (response.status() == MakeConsoleResponsePB::SUCCESS) {
    *console_id = response.console_id();
  }

  return true;
}

bool StartGame(int console_id,
               const std::shared_ptr<NetPlayServerService::StubInterface>& stub,
               StartGameResponsePB::Status* status) {
  StartGameRequestPB request;

  request.set_console_id(console_id);

  StartGameResponsePB response;
  grpc::ClientContext context;
  context.set_deadline(std::chrono::system_clock::now() +
                       std::chrono::milliseconds(5000));
  VLOG(3) << "Starting a game:\n" << request.DebugString();
  auto rpc_status = stub->StartGame(&context, request, &response);
  if (!rpc_status.ok()) {
    LOG(ERROR) << "StartGame: Failed with error message \""
               << rpc_status.error_message() << "\"";
    return false;
  }
  VLOG(3) << "Received a game start response:\n" << response.DebugString();

  *status = response.status();

  return true;
}

}  // namespace host_utils
