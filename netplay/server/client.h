#ifndef SERVER_CLIENT_H
#define SERVER_CLIENT_H

#include "base/netplayServiceProto.pb.h"
#include "server/Console.h"

namespace server {

class Player;
std::atomic_long client_id_generator;
enum ClientStatus {UNKNOWN, CREATED, READY, PLAYING, DONE}

class Client {
  public:
    virtual Client(const Console& console, const int delay);
    virtual ~Client() = default;
	
  long GetId();
	const Player& AddPlayer(Port port);
  vector<Port> GetPorts();
  
  private:
    int delay_;
    long id_;
	  const Conosle& console_;
	  ClientStatus status_;
	  std::vector<unique_ptr<Player>> players_;

} // namespace server

#endif // SERVER_CLIENT_H