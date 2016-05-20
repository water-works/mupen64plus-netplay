#include "server/netplay-server.h"

namespace server {

grpc::Status NetplayServer::Ping(grpc::ServerContext* context,
                                 const PingPB* request, PingPB* response) {
  return grpc::Status::OK;
}

grpc::Status NetplayServer::MakeConsole(grpc::ServerContext* context,
                                        const MakeConsoleRequestPB* request,
                                        MakeConsoleResponsePB* response) {
  return grpc::Status::OK;
}

grpc::Status PlugController(grpc::ServerContext* context,
                            const PlugControllerRequestPB* request,
                            PlugControllerResponsePB* response) {
  return grpc::Status::OK;
}

grpc::Status StartGame(grpc::ServerContext* context,
                       const StartGameRequestPB* request,
                       StartGameResponsePB* response) {
  return grpc::Status::OK;
}

grpc::Status SendEvent(
    grpc::ServerContext* context,
    grpc::ServerReaderWriter<IncomingEventPB, OutgoingEventPB>* stream) {
  return grpc::Status::OK;
}

grpc::Status ShutDownServer(grpc::ServerContext* context,
                            const ShutDownServerRequestPB* request,
                            ShutDownServerResponsePB* response) {
  return grpc::Status::OK;
}

}  // namespace server
