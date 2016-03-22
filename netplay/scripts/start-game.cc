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
  if (argc != 4) {
    LOG(INFO) << "Usage: start-game [hostname] [port] [console id]";
    return 1;
  }

  const std::string hostname = argv[1];
  const std::string port = argv[2];
  const int console_id = atoi(argv[3]);

  std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(
      hostname + ":" + port, grpc::InsecureChannelCredentials());
  std::shared_ptr<NetPlayServerService::StubInterface> stub =
      NetPlayServerService::NewStub(channel);

  StartGameResponsePB::Status status;
  if (!host_utils::StartGame(console_id, stub, &status)) {
    LOG(ERROR) << "StartGame call failed with error: \""
               << StartGameResponsePB::Status_Name(status) << "\"";
    return 1;
  }

  LOG(INFO) << "Started game on console ID " << console_id;

  return 0;
}
