#ifndef SERVER_CONSOLE_H_
#define SERVER_CONSOLE_H_

#include "base/netplayServiceProto.pb.h"
#include "base/netplayServiceProto.grpc.pb.h"

using std::vector;

namespace server {

typedef PlugControllerResponsePB_PortRejectionPB PortRejectionPB;

const std::vector<Port> ALL_PORTS 
    = {Port::PORT_1, Port::PORT_2, Port::PORT_3, Port::PORT_4};
class Client;

enum ConsoleStatus {UNKNOWN, CREATED, POWERED, DONE};

class Console {
 public:
   Console(long console_id);
   
   grpc::Status TryAddPlayers(int delay, vector<Port> requested_ports,
    vector<PortRejectionPB>* rejections,
    Client* client);
    
    Client* GetClientById(long id);

 private:
   const Client& MakeNewClient(const vector<Port>& ports, int delay);
   
   grpc::Status TryAddPlayersInternal(int delay, const vector<Port>& requested_ports);
 
   std::mutex port_lock_;
   ConsoleStatus console_status_;
   const long console_id_;
   std::map<Port, std::unique_ptr<Client>> client_port_map_;
};

}  // namespace server

#endif  // SERVER_CONSOLE_H_
