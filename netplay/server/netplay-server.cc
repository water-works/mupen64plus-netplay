#include "server/netplay-server.h"

namespace server {

NetplayServer::NetplayServer(bool debug_mode) : debug_mode_(debug_mode) {}

grpc::Status NetplayServer::Ping(grpc::ServerContext* context,
                                 const PingPB* request, PingPB* response) {
  return grpc::Status::OK;
}

grpc::Status NetplayServer::MakeConsole(grpc::ServerContext* context,
                                        const MakeConsoleRequestPB* request,
                                        MakeConsoleResponsePB* response) {
  const long console_id = ++console_id_generator_;

  {
    std::lock_guard<std::mutex> console_guard(console_lock_);
    consoles_.emplace(console_id,
                      std::unique_ptr<Console>(new Console(console_id)));
  }

  return grpc::Status::OK;
}

grpc::Status NetplayServer::PlugController(
    grpc::ServerContext* context, const PlugControllerRequestPB* request,
    PlugControllerResponsePB* response) {
  return grpc::Status::OK;
}

grpc::Status NetplayServer::StartGame(grpc::ServerContext* context,
                                      const StartGameRequestPB* request,
                                      StartGameResponsePB* response) {
  return grpc::Status::OK;
}

grpc::Status NetplayServer::SendEvent(
    grpc::ServerContext* context,
    grpc::ServerReaderWriter<IncomingEventPB, OutgoingEventPB>* stream) {
  return grpc::Status::OK;
}

grpc::Status NetplayServer::ShutDownServer(
    grpc::ServerContext* context, const ShutDownServerRequestPB* request,
    ShutDownServerResponsePB* response) {
  return grpc::Status::OK;
}

}  // namespace server
