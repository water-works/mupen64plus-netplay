#include "server/netplay-server.h"
#include "test-mocks.h"

namespace server {

template <typename StaticConsoleFactory>
NetplayServerImpl<StaticConsoleFactory>::NetplayServerImpl(bool debug_mode)
    : debug_mode_(debug_mode) {}

template <typename StaticConsoleFactory>
grpc::Status NetplayServerImpl<StaticConsoleFactory>::Ping(
    grpc::ServerContext* context, const PingPB* request, PingPB* response) {
  return grpc::Status::OK;
}

template <typename StaticConsoleFactory>
grpc::Status NetplayServerImpl<StaticConsoleFactory>::MakeConsole(
    grpc::ServerContext* context, const MakeConsoleRequestPB* request,
    MakeConsoleResponsePB* response) {
  const long console_id = ++console_id_generator_;

  {
    std::lock_guard<std::mutex> console_guard(console_lock_);
    consoles_.emplace(
        console_id,
        std::unique_ptr<ConsoleType>(StaticConsoleFactory::MakeConsole(
            console_id, request->rom_file_md5())));
  }

  response->set_status(MakeConsoleResponsePB::SUCCESS);
  response->set_console_id(console_id);

  return grpc::Status::OK;
}

template <typename StaticConsoleFactory>
grpc::Status NetplayServerImpl<StaticConsoleFactory>::PlugController(
    grpc::ServerContext* context, const PlugControllerRequestPB* request,
    PlugControllerResponsePB* response) {
  std::lock_guard<std::mutex> guard(console_lock_);

  const auto console_it = consoles_.find(request->console_id());
  if (console_it == consoles_.end()) {
    response->set_console_id(request->console_id());
    response->set_status(PlugControllerResponsePB::NO_SUCH_CONSOLE);
    return grpc::Status::OK;
  }

  std::unique_ptr<ConsoleType>& console = console_it->second;

  console->RequestPortMapping(*request, response);

  return grpc::Status::OK;
}

template <typename StaticConsoleFactory>
grpc::Status NetplayServerImpl<StaticConsoleFactory>::StartGame(
    grpc::ServerContext* context, const StartGameRequestPB* request,
    StartGameResponsePB* response) {
  response->set_console_id(request->console_id());

  {
    std::lock_guard<std::mutex> guard(console_lock_);
    const auto& it = consoles_.find(request->console_id());
    if (it == consoles_.end()) {
      response->set_status(StartGameResponsePB::NO_SUCH_CONSOLE);
      return grpc::Status::OK;
    }
    if (!it->second->ClientsPresentAndReady()) {
      response->set_status(StartGameResponsePB::NOT_ALL_CLIENTS_READY);
      return grpc::Status::OK;
    }
  }

  return grpc::Status::OK;
}

template <typename StaticConsoleFactory>
grpc::Status NetplayServerImpl<StaticConsoleFactory>::SendEvent(
    grpc::ServerContext* context,
    grpc::ServerReaderWriter<IncomingEventPB, OutgoingEventPB>* stream) {
  return grpc::Status::OK;
}

template <typename StaticConsoleFactory>
grpc::Status NetplayServerImpl<StaticConsoleFactory>::ShutDownServer(
    grpc::ServerContext* context, const ShutDownServerRequestPB* request,
    ShutDownServerResponsePB* response) {
  return grpc::Status::OK;
}

template class NetplayServerImpl<ConsoleFactory>;
template class NetplayServerImpl<MockConsoleFactory>;

}  // namespace server
