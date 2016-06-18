#ifndef CLIENT_H_
#define CLIENT_H_

#include <memory>
#include <string>
#include <vector>

#include "base/netplayServiceProto.grpc.pb.h"
#include "base/timings.pb.h"
#include "client/button-coder-interface.h"
#include "client/event-stream-handler.h"

template <typename ButtonsType>
class NetplayClientInterface {
 public:
  virtual ~NetplayClientInterface() {}
  virtual bool PlugControllers(int64_t console_id,
                               const std::vector<Port>& ports,
                               PlugControllerResponsePB::Status* status) = 0;
  virtual int delay_frames() const = 0;
  virtual int64_t console_id() const = 0;
  virtual int64_t client_id() const = 0;
  virtual const std::vector<Port>& local_ports() const = 0;
  virtual std::shared_ptr<NetPlayServerService::StubInterface> stub() const = 0;

  virtual TimingsPB* mutable_timings() = 0;

  // To mock out MakeEventStreamHandler, override MakeEventStreamHandlerRaw.
  std::unique_ptr<EventStreamHandlerInterface<ButtonsType>>
  MakeEventStreamHandler() {
    return std::unique_ptr<EventStreamHandlerInterface<ButtonsType>>(
        MakeEventStreamHandlerRaw());
  }
 protected:
  virtual EventStreamHandlerInterface<ButtonsType>*
  MakeEventStreamHandlerRaw() = 0;
};

// Implements sending and receiving Netplay controller inputs.
template <typename ButtonsType>
class NetplayClient : public NetplayClientInterface<ButtonsType> {
 public:
  static const int kNoConsoleId = -1;

  // A note on return convention for client methods that call RPCs: These
  // methods return true if all RPC calls succeed. In the case of RPC success,
  // these methods populate *status with the appropriate server status code.

  // Creates a new client with the given local frame delay and console ID.
  NetplayClient(std::shared_ptr<NetPlayServerService::StubInterface> stub,
                std::unique_ptr<ButtonCoderInterface<ButtonsType>> coder,
                int delay_frames);

  // Request that the given ports be plugged into the server's virtual console.
  // Returns the resulting status code returned from the server for this
  // request.
  bool PlugControllers(int64_t console_id, const std::vector<Port>& ports,
                       PlugControllerResponsePB::Status* status) override;

  // Accessors
  int delay_frames() const override { return delay_frames_; }
  int64_t console_id() const override { return console_id_; }
  int64_t client_id() const override { return client_id_; }
  const std::vector<Port>& local_ports() const override { return local_ports_; }
  std::shared_ptr<NetPlayServerService::StubInterface> stub() const override {
    return stub_;
  }

  TimingsPB* mutable_timings() override { return &timings_; }

 protected:
  // Create an event stream handler that will receive and transmit game events.
  // The return value will satisfy "val == nullptr" on failure.
  EventStreamHandlerInterface<ButtonsType>* MakeEventStreamHandlerRaw()
      override;

 private:
  const int delay_frames_;
  std::unique_ptr<ButtonCoderInterface<ButtonsType>> coder_;
  // Client ID and console ID are set by the PlugControllers method.
  int64_t console_id_;
  int64_t client_id_;

  std::vector<Port> local_ports_;

  TimingsPB timings_;

  std::shared_ptr<NetPlayServerService::StubInterface> stub_;
};

#include "client.hpp"

#endif  // CLIENT_H_
