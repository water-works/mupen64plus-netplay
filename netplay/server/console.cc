#include "server/console.h"

#include <array>
#include <map>

namespace server {

Console::Console(int64_t console_id, const std::string& rom_file_md5)
    : console_id_(console_id),
      rom_file_md5_(rom_file_md5),
      client_id_generator_(0) {}

Console::Client::Client(int64_t client_id_tmp, int64_t delay_frames_tmp)
    : client_id(client_id_tmp),
      delay_frames(delay_frames_tmp),
      stream(nullptr) {}

// -----------------------------------------------------------------------------
// RequestPortMapping

// Helper methods.
namespace {

// Defines the following sort order:
//  - PORT_1
//  - PORT_2
//  - PORT_3
//  - PORT_4
//  - PORT_ANY
// Note UNKNOWN's sort ordering is undefined, but we don't care since we skip it
// in our loop.
bool RequestPortMappingPortLessThan(Port a, Port b) {
  if (a == Port::PORT_ANY) {
    return false;
  } else {
    return a < b;
  }
}

bool IsPortRequest(Port p) {
  return p == Port::PORT_1 || p == Port::PORT_2 || p == Port::PORT_3 ||
         p == Port::PORT_4 || p == Port::PORT_ANY;
}

}  // namespace

Port Console::GetUnmappedPort(const PortToClientMap& clients,
                              const std::set<Port>& allocated_ports,
                              Port port) {
  switch (port) {
    case Port::PORT_ANY:
      for (const Port p :
           {Port::PORT_1, Port::PORT_2, Port::PORT_3, Port::PORT_4}) {
        if (clients.find(p) == clients.end() &&
            allocated_ports.find(p) == allocated_ports.end()) {
          return p;
        }
      }
      return Port::UNKNOWN;
    case Port::PORT_1:
    case Port::PORT_2:
    case Port::PORT_3:
    case Port::PORT_4:
      if (clients.find(port) == clients.end() &&
          allocated_ports.find(port) == allocated_ports.end()) {
        return port;
      } else {
        return Port::UNKNOWN;
      }
    default:
      return Port::UNKNOWN;
  }
}

void Console::RequestPortMapping(const PlugControllerRequestPB& request,
                                 PlugControllerResponsePB* response) {
  response->set_console_id(console_id_);

  // The ROM MD5 doesn't match
  if (request.rom_file_md5() != rom_file_md5_) {
    response->set_status(PlugControllerResponsePB::ROM_MD5_MISMATCH);
    return;
  }

  // No ports were requested.
  if (!IsPortRequest(request.requested_port_1()) &&
      !IsPortRequest(request.requested_port_2()) &&
      !IsPortRequest(request.requested_port_3()) &&
      !IsPortRequest(request.requested_port_4())) {
    response->set_status(PlugControllerResponsePB::NO_PORTS_REQUESTED);
    return;
  }

  // Sort the ports so that the specifically-named ports are ordered first.
  std::array<Port, 4> req_ports(
      {request.requested_port_1(), request.requested_port_2(),
       request.requested_port_3(), request.requested_port_4()});
  std::sort(req_ports.begin(), req_ports.end(), RequestPortMappingPortLessThan);

  std::set<Port> allocated_ports;

  int64_t client_id = -1;
  {
    std::lock_guard<std::mutex> clients_guard(client_lock_);

    client_id = ++client_id_generator_;

    // Find suitable unallocated ports for all the port requests.
    for (const Port req_port : req_ports) {
      if (req_port != Port::PORT_1 && req_port != Port::PORT_2 &&
          req_port != Port::PORT_3 && req_port != Port::PORT_4 &&
          req_port != Port::PORT_ANY) {
        continue;
      }
      Port port = GetUnmappedPort(clients_, allocated_ports, req_port);
      if (port != Port::UNKNOWN) {
        allocated_ports.insert(port);
      } else {
        auto* rejection = response->add_port_rejections();
        rejection->set_port(req_port);
        rejection->set_reason(
            PlugControllerResponsePB::PortRejectionPB::PORT_ALREADY_OCCUPIED);
        break;
      }
    }

    // If any port request was rejected, return early. Note we decrement the
    // client ID generator.
    if (!response->port_rejections().empty()) {
      response->set_status(PlugControllerResponsePB::PORT_REQUEST_REJECTED);
      client_id_generator_--;
      return;
    }

    // Update clients_ with the freshly allocated ports.
    for (Port p : allocated_ports) {
      clients_.emplace(
          std::make_pair(p, Client(client_id, request.delay_frames())));
    }
  }

  for (const Port p : allocated_ports) {
    response->add_port(p);
  }

  response->set_status(PlugControllerResponsePB::SUCCESS);
  response->set_client_id(client_id);
}

// -----------------------------------------------------------------------------
// RegisterStream and HandleEvent

// Assumes the caller holds client_lock_
void Console::GetClientsForClientId(int64_t client_id,
                                    std::vector<Client*>* clients) {
  for (auto& it : clients_) {
    if (it.second.client_id == client_id) {
      clients->push_back(&it.second);
    }
  }
}

bool Console::RegisterStream(int64_t client_id,
                             grpc::WriterInterface<IncomingEventPB>* stream) {
  bool found_client = false;

  {
    std::lock_guard<std::mutex> client_guard(client_lock_);

    for (auto& it : clients_) {
      if (it.second.client_id == client_id) {
        it.second.stream = stream;
        found_client = true;
      }
    }
  }

  return found_client;;
}

// Assumes the caller holds client_lock_
void Console::PopulateKeyPressesForClient(const OutgoingEventPB& received,
                                          IncomingEventPB* forwarded,
                                          int64_t client_id) {
  for (const auto& key_press : received.key_press()) {
    const auto it = clients_.find(key_press.port());
    if (it != clients_.end() && it->second.client_id != client_id) {
      (*forwarded->add_key_press()) = key_press;
    }
  }
}

// Handles the given event proto.
bool Console::HandleEvent(const OutgoingEventPB& event) {
  for (const auto& key_press : event.key_press()) {
    if (key_press.console_id() != console_id_) {
      return false;
    }
  }

  if (!event.key_press().empty()) {
    std::map<grpc::WriterInterface<IncomingEventPB>*, IncomingEventPB>
        deferred_writes;

    std::lock_guard<std::mutex> client_guard(client_lock_);

    // Keep track of the streams we've already written to so that we don't write 
    // multiple messages to the same client.
    std::set<const grpc::WriterInterface<IncomingEventPB>*> written_streams;
    for (const auto& it : clients_) {
      const Client& client = it.second;
      const auto& write = deferred_writes.find(client.stream);
      if (write != deferred_writes.end()) {
        continue;
      }

      IncomingEventPB forwarded_event;
      PopulateKeyPressesForClient(event, &deferred_writes[client.stream],
                                  client.client_id);
    }

    // TODO: Expose and export this deferred write map to an asynchronous writer
    // instead of performing network writes while holding a lock.
    for (const auto& it : deferred_writes) {
      if (!it.second.key_press().empty()) {
        it.first->Write(it.second);
      }
    }
  }

  return true;
}

}  // namespace server
