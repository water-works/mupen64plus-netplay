#include "server/player.h"

namespace server {

Player::Player(Port port, const Client& client) : port_(port), client_(client) {}

const Port& Player::GetPort() {
  return port_;
}


} // namespace server