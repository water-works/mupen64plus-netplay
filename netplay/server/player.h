#ifndef SERVER_PLAYER_H
#define SERVER_PLAYER_H

#include "base/netplayServiceProto.pb.h"
#include "server/Client.h"

namespace server {

class Player {
  public:
  virtual Player(Port port, const Client& client);
  virtual ~Player() = default;
  
  public const Port& getPort();
  
  public const Client& GetClient();
  
  public int GetId();
  
  private:
    long id_;
	const Port& port_;
	const Client& client_;
};

} // namespace server

#endif // SERVER_PLAYER_H