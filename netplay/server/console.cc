#include "server/console.h"

namespace server {
namespace {

// We want to sort ports in ascending order, with
// PORT_ANY last in the list.
bool PortCompare(const Port& a, const Port& b) {
  if (a == Port::PORT_ANY) {
    return false;
  }
  return a < b;
}

} // namespace

Console::Console(long console_id) : console_id_(console_id) {}

// TODO: complete this function.
Client* Console::GetClientById(long id) {
  for (auto& client_pair : client_port_map_) {    
  }
  return nullptr;
}

grpc::Status Console::TryAddPlayers(int delay, 
    vector<Port> requested_ports, vector<PortRejectionPB>* rejections, 
    Client* client) {
  
  if (requested_ports.empty()) {
    return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "No ports requested.");
  }
  
  std::lock_guard<std::mutex> lock(port_lock_);

  std::deque<Port> available_ports;
  // Collect all available ports.
  for (const Port& port : ALL_PORTS) {
    if (client_port_map_.count(port) != 1) {
      available_ports.push_back(port);
    }
  }
  
  vector<Port> assigned_ports;
  bool can_assign = true;
  
  vector<Port> sorted_requests = std::sort(requested_ports.begin(), requested_ports.end(), PortCompare);
  for (const Port& request_port : sorted_requests) {
    if (request_port == Port.UNKNOWN || request_port == Port::UNRECOGNIZED) {
      continue;
    }
    PortRejectionPB reject_reason;
    reject_reason.set_port(request_port);
    reject_reason.set_reason(Reason::ACCEPTABLE);      
    if (request_port == Port::PORT_ANY) {
      if (available_ports.size() < 1) {
        can_assign = false;
        reject_reason.set_reason(Reason::ALL_PORTS_OCCUPIED);
      }
      Port next_available_port = available_ports.pop_front();
      reject_reason.set_port(next_available_port);
      assigned_ports.push_back(next_available_port);
    }
    else {
      if (client_port_map_.count(request_port) == 1) {
        can_assign = false;
        reject_reason.set_port(request_port);
        reject_reason.set_reason(Reason::PORT_ALREADY_OCCUPIED);        
      } else {
        assigned_ports.push_back(request_port);
        for (auto i = available_ports.begin(); i != available_ports.end(); i++) {
          if (available_ports[i] = request_port) {
            available_ports.erase(i);
          }
        }
      }
    }
    rejections->push_back(reject_reason);
  }
  if (!canAssign) {
    return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "Cannot assign ports.");
  }
  MakeNewClient(assigned_ports, delay);
  return grpc::Status::OK;
}

const Client& Console::MakeNewClient(const vector<Port>& ports, int delay) {
  auto client = std::unique_ptr<Client>(new Client(*this, delay);
  for (const Port& port : ports) {
    client_port_map_.emplace(port, std::move(client));
    client->AddPlayer(port);
  }
  return *client;
}


}  // namespace server
