#include "server/netplay-server.h"

namespace server {

using PlugControllerResponsePB::PortRejectionPB;

namespace {

grpc::Status AddRejections(const vector<PortRejectionPB>& rejects, PlugControllerResponsePB* response) {
  if (rejects.empty()) {
    response->set_status(PlugControllerResponsePB.Status::NO_PORTS_REQUESTED);
  } else {
    response->set_status(PlugControllerResponsePB.Status::PORT_REQUEST_REJECTED);
    for (auto reject : rejects) {
    response->add_port_rejections() = reject;
    }
  }
  return grpc::Status::OK;
}

} // namespace

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
  response->set_console_id(console_id);
  response->set_status(Status.SUCCESS);
  
  return grpc::Status::OK;
}

grpc::Status NetplayServer::PlugController(
    grpc::ServerContext* context, const PlugControllerRequestPB* request,
    PlugControllerResponsePB* response) {
  if (consoles_.count(request->console_id()) != 1) {
     return grpc::Status(grpc::StatusCode::NOT_FOUND, 
     "Console id: " + request->console_id() + " not found.");
  }
  Client* new_client;
  vector<Port> requested_ports = {request->requested_port_1(), request->requested_port_2(),
    request->requested_port_3(), request->requested_port_4()}
  vector<PortRejectionPB> rejects;
  auto status = consoles_[request->console_id()]->TryAddPlayers(request->delay(),
    requested_ports, &rejects, new_client);
  if (!status.ok()) {
    return AddRejections(rejects, response);
  }
  response->set_status(PlugControllerResponsePB.Status.SUCCESS);
  response->set_console_id(request->console_id());
  response->set_client_id(new_client->GetId());
  for (const auto& port : new_client->GetPorts()) {
    *response->add_port() = port;
  }  
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
