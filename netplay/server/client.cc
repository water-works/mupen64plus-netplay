#include "server/client.h"

namespace server {

Client::Client(const Console& console, const int delay) : console_(console) 
                                                          delay_(delay) {
  status_ = ClientStatus.CREATED;
  id_ = ++client_id_generator;
}

long Client::GetId() {
  return id_;
}

vector<Port> Client::GetPorts() {
  vector<Port> ports;
  for (auto player : players_) {
    ports.push_back(player.GetPort());
  }
  return ports;
}

const Player& Client::AddPlayer(Port port) {
  auto player = std::unique_ptr<Player>(new Player(port, *this));
  players_.push_back(std::move(player));
  return *players_.back();

}

} // namespace server