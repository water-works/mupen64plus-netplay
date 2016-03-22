#include <iostream>
#include <memory>
#include <string>

#include "base/netplayServiceProto.pb.h"
#include "base/netplayServiceProto.grpc.pb.h"
#include "client/client.h"
#include "client/host-utils.h"
#include "glog/logging.h"
#include "grpc++/channel.h"

int main(int argc, char** argv) {
  if (argc != 3) {
    LOG(INFO) << "Usage: mkconsole [hostname] [port]";
    return 1;
  }

  const std::string hostname = argv[1];
  const std::string port = argv[2];

  MakeConsoleResponsePB::Status status;
  int console_id;

  std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(
      hostname + ":" + port, grpc::InsecureChannelCredentials());
  std::shared_ptr<NetPlayServerService::StubInterface> stub =
      NetPlayServerService::NewStub(channel);

  if (!host_utils::MakeConsole("dummy-console", "dummy-rom-name", stub, &status,
                               &console_id)) {
    LOG(ERROR) << "MakeConsole call failed with status: \""
               << MakeConsoleResponsePB::Status_Name(status) << "\"";
    return 1;
  }

  LOG(INFO) << "Successfully created console on server " << hostname << ":"
            << port << ". Console ID is: " << console_id;

  return 0;
}
