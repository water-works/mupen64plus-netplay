// included by client.h

#include <chrono>
#include <iostream>

#include "base/netplayServiceProto.grpc.pb.h"
#include "client/utils.h"
#include "glog/logging.h"
#include "grpc++/create_channel.h"
#include "grpc++/client_context.h"

template <typename ButtonsType>
NetplayClient<ButtonsType>::NetplayClient(
    std::shared_ptr<NetPlayServerService::StubInterface> stub,
    std::unique_ptr<ButtonCoderInterface<ButtonsType>> coder, int delay_frames,
    int console_id)
    : delay_frames_(delay_frames),
      console_id_(console_id),
      coder_(std::move(coder)),
      client_id_(-1),
      stub_(stub) {}

template <typename ButtonsType>
bool NetplayClient<ButtonsType>::PlugControllers(
    const std::vector<Port>& local_ports,
    PlugControllerResponsePB::Status* status) {
  PlugControllerRequestPB request;
  request.set_console_id(console_id_);
  request.set_delay_frames(delay_frames_);

  // Populate all the requested local_ports, and return false if the size is not
  // in
  // [0, 4].
  switch (local_ports.size()) {
    case 4:
      request.set_requested_port_4(local_ports[3]);
    // Fallthrough intended
    case 3:
      request.set_requested_port_3(local_ports[2]);
    // Fallthrough intended
    case 2:
      request.set_requested_port_2(local_ports[1]);
    // Fallthrough intended
    case 1:
      request.set_requested_port_1(local_ports[0]);
      break;
    default: {
      LOG(ERROR) << "Invalid local_ports length: " << local_ports.size()
                 << std::endl;
      return false;
    }
  }

  PlugControllerResponsePB response;
  grpc::ClientContext context;
  context.set_deadline(std::chrono::system_clock::now() +
                       std::chrono::milliseconds(5000));
  VLOG(3) << "Requesting controllers with message: \n" << request.DebugString();

  mutable_timings()->add_event()->set_plug_controller_request(
      client_utils::now_nanos());
  grpc::Status rpc_status = stub_->PlugController(&context, request, &response);
  mutable_timings()->add_event()->set_plug_controller_response(
      client_utils::now_nanos());
  if (!rpc_status.ok()) {
    LOG(ERROR) << "RPC failed with error message:\""
               << rpc_status.error_message() << "\"";
    return false;
  }

  VLOG(3) << "Received plug controllers response:\n" << response.DebugString();

  if (response.console_id() != console_id_) {
    LOG(ERROR) << "Console ID doesn't match. Expected " << console_id_
               << ", but saw " << response.console_id();
    return false;
  }

  client_id_ = response.client_id();
  for (const int port_int : response.port()) {
    Port port = static_cast<Port>(port_int);
    local_ports_.push_back(port);
  }

  *status = response.status();
  return true;
}

template <typename ButtonsType>
EventStreamHandlerInterface<ButtonsType>*
NetplayClient<ButtonsType>::MakeEventStreamHandlerRaw() {
  return new EventStreamHandler<ButtonsType>(
      console_id_, client_id_, local_ports_, &timings_, coder_.get(), stub_);
}
