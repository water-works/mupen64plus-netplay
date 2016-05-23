#ifndef SERVER_NETPLAY_SERVER_H_
#define SERVER_NETPLAY_SERVER_H_

#include <atomic>
#include <map>
#include <memory>
#include <mutex>

#include "base/netplayServiceProto.pb.h"
#include "base/netplayServiceProto.grpc.pb.h"
#include "gtest/gtest.h"
#include "server/console.h"

namespace server {

// Implements the netplay server implementation. This class primarily concerns 
// itself with setting up and maintaining consoles. Services include:
//  - Creating consoles.
//  - Handling incoming clients and allocating them to consoles.
//  - Accepting incoming streams.
//
// This class is thread-safe.
class NetplayServer : public NetPlayServerService::Service {
 public:
  NetplayServer(bool debug_mode);

  // Returns an empty response.
  grpc::Status Ping(grpc::ServerContext* context, const PingPB* request,
                    PingPB* response) override;

  // Create a new console and insert it into the console map.
  //
  // Takes console_lock_ to insert the new console into the console map.
  //
  // Responds SUCCESS on success, UNSPECIFIED_FAILURE on any failure.
  grpc::Status MakeConsole(grpc::ServerContext* context,
                           const MakeConsoleRequestPB* request,
                           MakeConsoleResponsePB* response) override;

  // Request that the client be added to an existing console.
  //
  // Takes console_lock_ to compute the controller mapping and fetch a pointer 
  // to the relevant console.
  //
  // Responds SUCCESS on success. On failure returns:
  //  - ROM_MD5_MISMATCH if the client's MD5 ROM is specified and does not match 
  //    the ROM MD5 with which the client was initialized.
  //  - NO_PORTS_REQUESTED if the client didn't request any ports.
  //  - PORT_REQUEST_REJECTED if the requested port mapping could not be 
  //    satisfied. May also populate the port_rejections field of the response 
  //    proto to give a friendly indication of why ports allocation failed.
  grpc::Status PlugController(grpc::ServerContext* context,
                              const PlugControllerRequestPB* request,
                              PlugControllerResponsePB* response) override;

  // Attemps to start a console. Responds SUCCESS on success, 
  // UNSPECIFIED_FAILURE on all failures.
  //
  // Takes console_lock_ to find the appropriate console.
  //
  // On success, sends the StartGame event to all clients connected to the 
  // specified console. This event will describe the port allocation delay frame 
  // data for the connected ports in the response's connected_ports field.
  //
  // TODO(alexgolec): Make the rejection reasons more useful.
  grpc::Status StartGame(grpc::ServerContext* context,
                         const StartGameRequestPB* request,
                         StartGameResponsePB* response) override;

  // Initiates a bidirectional stream between the client. Starts listening on 
  // the event stream and passes all incoming events to the console. Note the 
  // stream pointer is lent to the console to allow it to broadcast events 
  // across all streams.
  //
  // Note we cannot immediately tie the stream to the appropriate console ID. In 
  // other words, we cannot determine which console this stream beint64_ts to 
  // simply by looking at the stream object itself. Instead, we expect the 
  // stream to identify itself with a ClientReady event. All events received 
  // before this event will be responded to with an InvalidData event. Once this 
  // event is received, the stream will be forwarded to the appropriate console.
  //
  // Takes console_lock_ once to fetch a pointer to the relevant console. Does 
  // not take console_lock_ from within the stream handling loop.
  //
  // Note this method assumes all incoming messages are related to the same 
  // console ID. All incoming messages with irrelevant console ID are responded 
  // to with an InvalidData event.
  grpc::Status SendEvent(
      grpc::ServerContext* context,
      grpc::ServerReaderWriter<IncomingEventPB, OutgoingEventPB>* stream)
      override;

  // Requests that the server shut itself down. If the server is is in debug 
  // mode, return an affirmative response, halt all games, and shut down the 
  // server. Otherwise ignore the message.
  grpc::Status ShutDownServer(grpc::ServerContext* context,
                              const ShutDownServerRequestPB* request,
                              ShutDownServerResponsePB* response) override;

 private:
  const bool debug_mode_;

  std::atomic_long console_id_generator_;

  static_assert(sizeof(std::atomic_long) == sizeof(int64_t),
                "std::atomic_long must be exactly 64 bits wide");

  std::mutex console_lock_;
  // Begin guarded by console_lock_
  std::map<int64_t, std::unique_ptr<Console>> consoles_;
  // End guarded by console_lock_

  FRIEND_TEST(NetplayServerTest, MakeConsoleSuccess);
};

}  // namespace server

#endif  // SERVER_NETPLAY_SERVER_H_
