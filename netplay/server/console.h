#ifndef SERVER_CONSOLE_H_
#define SERVER_CONSOLE_H_

#include <map>
#include <mutex>
#include <set>

#include "base/netplayServiceProto.pb.h"
#include "base/netplayServiceProto.grpc.pb.h"
#include "gtest/gtest.h"

namespace server {

class Console {
 public:
  Console(int64_t console_id, const std::string& rom_file_md5);

  // Attempts to allocate the port mapping specified by the request. On success, 
  // populates response->status with SUCCESS, otherwise populates it with the 
  // failure reason and rejection reasons.
  void RequestPortMapping(const PlugControllerRequestPB& request,
                          PlugControllerResponsePB* response);

  // Registers the output stream with the specified client ID. Returns false if 
  // the client could not be registered.
  bool RegisterStream(int64_t client_id,
                      grpc::WriterInterface<IncomingEventPB>* stream);

  // Returns true if all clients in the client map have streams registered. 
  // Returns false if:
  //  - There are no clients.
  //  - Not all clients have their stream field set.
  bool ClientsPresentAndReady();

  // Handles the given event proto.
  bool HandleEvent(const OutgoingEventPB& event);

 private:
  struct Client;
  typedef std::map<Port, Client> PortToClientMap;

  // Internal representation of a client. Note all fields except stream are 
  // const. The stream is initialized to nullptr and later populated to point to 
  // a stream received by the server once the stream is opened and the client 
  // identifies themselves.
  struct Client {
    Client(int64_t client_id, int64_t delay_frames);

    const int64_t client_id;
    const int32_t delay_frames;

    // Borrowed pointer.
    grpc::WriterInterface<IncomingEventPB>* stream;
  };

  // Helper method for RequestPortMapping. Returns an unallocated port number in 
  // the given mapping, or UNKNOWN if no satisfactory port can be found.
  Port GetUnmappedPort(const PortToClientMap& clients,
                       const std::set<Port>& allocated_ports, Port port);

  // Copy the keypresses that are unrelated to the given client into the 
  // forwarded event proto. This method assumes the caller holds client_lock.
  void PopulateKeyPressesForClient(const OutgoingEventPB& received,
      IncomingEventPB* forwarded, int64_t client_id);

  const int64_t console_id_;
  const std::string rom_file_md5_;

  std::mutex client_lock_;
  // Begin guarded by client_lock_
  int64_t client_id_generator_;
  PortToClientMap clients_;
  // End guarded by client_lock_

  FRIEND_TEST(ConsoleTest, RegisterStreamSuccess);
};

}  // namespace server

#endif  // SERVER_CONSOLE_H_
