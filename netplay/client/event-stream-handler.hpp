#include "event-stream-handler.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <set>

#include "glog/logging.h"

#include "client/utils.h"

template <typename ButtonsType>
EventStreamHandler<ButtonsType>::EventStreamHandler(
    int console_id, int client_id, const std::vector<Port> local_ports,
    TimingsPB* timings, const ButtonCoderInterface<ButtonsType>* coder,
    std::unique_ptr<CompletionQueueWrapper> cq,
    std::shared_ptr<NetPlayServerService::StubInterface> stub)
    : console_id_(console_id),
      client_id_(client_id),
      local_ports_(local_ports.begin(), local_ports.end()),
      timings_(timings),
      coder_(*coder),
      stub_(stub),
      cq_(std::move(cq)),
      status_(HandlerStatus::NOT_YET_STARTED) {
  if (console_id_ <= 0) {
    LOG(ERROR) << "invalid console_id: " << console_id_;
    std::abort();
  }
  if (client_id_ <= 0) {
    LOG(ERROR) << "invalid client_id: " << client_id_;
    std::abort();
  }
  if (local_ports.size() > 4) {
    LOG(ERROR) << "local_ports has too many elements: " << local_ports.size();
    std::abort();
  }
  // Note local_ports_ is a std::set
  if (local_ports_.size() != local_ports.size()) {
    LOG(ERROR) << "local_ports contains duplicate values";
    std::abort();
  }
  if (std::find(local_ports.begin(), local_ports.end(), PORT_ANY) !=
      local_ports.end()) {
    LOG(ERROR) << "local_ports contains PORT_ANY";
    std::abort();
  }
}

// -----------------------------------------------------------------------------
// ClientReady and WaitForConsoleStart and their helpers

namespace {

bool ExpectTag(std::unique_ptr<CompletionQueueWrapper>& cq,
               const void* expected_tag) {
  void* tag;
  bool ok = false;
  cq->Next(&tag, &ok);
  const bool success = ok && tag == expected_tag;
  if (!ok) {
    LOG(ERROR) << "Failed to successfully get next event from stream";
    return false;
  }
  if (tag != expected_tag) {
    LOG(ERROR) << "Received unexpected tag " << tag << " (expected "
               << expected_tag << ")";
    return false;
  }
  return true;
}

}  // namespace

template <typename ButtonsType>
void* EventStreamHandler<ButtonsType>::ClientReady() {
  stream_ = stub_->AsyncSendEvent(&stream_context_, &cq_->cq, nullptr);
  return nullptr;
}

template <typename ButtonsType>
bool EventStreamHandler<ButtonsType>::WaitForConsoleStart(void* tag) {
  if (!ExpectTag(cq_, tag)) {
    return false;
  }

  // Notify the server we are ready to start the game.
  OutgoingEventPB client_ready_event;
  ClientReadyPB* client_ready = client_ready_event.mutable_client_ready();
  client_ready->set_console_id(console_id_);
  client_ready->set_client_id(client_id_);

  VLOG(3) << "Writing client ready request to stream:\n"
          << client_ready_event.DebugString();

  timings_->add_event()->set_client_ready_sync_write_start(
      client_utils::now_nanos());
  bool success = SyncWrite(client_ready_event);
  timings_->add_event()->set_client_ready_sync_write_finish(
      client_utils::now_nanos());

  if (!success) {
    LOG(ERROR) << "Failed to write client ready request: "
               << client_ready_event.DebugString();
    return false;
  }

  // Wait for the server to give us the combined start signal and console
  // configuration.
  VLOG(3) << "Expecting start game notification";
  IncomingEventPB start_game_event;

  timings_->add_event()->set_start_game_event_read_start(
      client_utils::now_nanos());
  success = SyncRead(&start_game_event);
  timings_->add_event()->set_start_game_event_read_finish(
      client_utils::now_nanos());

  if (!success) {
    LOG(ERROR) << "Failed to read event. Expected a StartGamePB.";
    return false;
  }
  VLOG(3) << "Received start game notification:\n"
          << start_game_event.DebugString();

  if (start_game_event.has_stop_console()) {
    LOG(ERROR) << "Console stopped before it was started: "
               << start_game_event.DebugString();
    return false;
  }
  if (!start_game_event.has_start_game()) {
    LOG(ERROR)
        << "Expected a StartGamePB, but instead saw the following event proto: "
        << start_game_event.DebugString();
    return false;
  }

  const StartGamePB& start_game = start_game_event.start_game();
  if (start_game.console_id() <= 0 || start_game.console_id() != console_id_) {
    LOG(ERROR) << "Server returned unexpected console_id: "
               << start_game.DebugString();
    return false;
  }

  if (!InitializeQueues(start_game.connected_ports())) {
    // Error already logged.
    input_queues_.clear();
    return false;
  }

  return true;
}

template <typename ButtonsType>
bool EventStreamHandler<ButtonsType>::InitializeQueues(
    const google::protobuf::RepeatedPtrField<StartGamePB::ConnectedPortPB>&
        ports) {
  if (ports.size() > 4) {
    LOG(ERROR) << "Too many ports. Expected 0 to 4, saw " << ports.size();
    return false;
  }

  std::set<Port> handled_ports;

  int local_ports_connected = 0;

  for (const StartGamePB::ConnectedPortPB connected_port : ports) {
    Port port_id = connected_port.port();

    if (port_id == PORT_ANY) {
      LOG(ERROR)
          << "Server returned connected port with unexpected value PORT_ANY.";
      return false;
    }

    if (handled_ports.find(port_id) != handled_ports.end()) {
      LOG(ERROR) << "Duplicate port returned from server: "
                 << Port_Name(port_id);
      return false;
    }

    // Actually initialize the port as either local or remote.
    if (local_ports_.find(port_id) != local_ports_.end()) {
      VLOG(3) << "Inserting local queue for connected port:\n"
              << connected_port.DebugString();
      input_queues_.emplace(port_id, std::unique_ptr<ButtonsInputQueue>(
                                         ButtonsInputQueue::MakeLocalQueue(
                                             connected_port.delay_frames())));
      ++local_ports_connected;
    } else {
      VLOG(3) << "Inserting remote queue for connected port:\n"
              << connected_port.DebugString();
      input_queues_.emplace(port_id, std::unique_ptr<ButtonsInputQueue>(
                                         ButtonsInputQueue::MakeRemoteQueue(
                                             connected_port.delay_frames())));
    }

    handled_ports.insert(port_id);
  }

  // Make sure all local ports were connected.
  if (local_ports_.size() != local_ports_connected) {
    LOG(ERROR) << "Failed to connect local ports: ";
    for (const Port local_port : local_ports_) {
      if (handled_ports.find(local_port) == handled_ports.end()) {
        LOG(ERROR) << Port_Name(local_port);
      }
    }

    return false;
  }

  return true;
}

// -----------------------------------------------------------------------------
// PutButtons and Helpers

template <typename ButtonsType>
typename EventStreamHandler<ButtonsType>::PutButtonsStatus
EventStreamHandler<ButtonsType>::PutButtons(const std::vector<
    EventStreamHandler<ButtonsType>::ButtonsFrameTuple>& buttons_tuples) {
  OutgoingEventPB event;

  for (const auto& buttons_tuple : buttons_tuples) {
    const Port port = std::get<0>(buttons_tuple);
    const int frame = std::get<1>(buttons_tuple);
    const ButtonsType& buttons = std::get<2>(buttons_tuple);

    auto queue_it = input_queues_.find(port);
    if (queue_it == input_queues_.end()) {
      LOG(ERROR) << "Attempted to send buttons for disconnected port "
                 << Port_Name(port);
      return PutButtonsStatus::NO_SUCH_PORT;
    }
    const std::unique_ptr<ButtonsInputQueue>& queue = queue_it->second;

    VLOG(3) << "Inserting buttons into a the queue for port " << Port_Name(port)
            << " and frame number " << frame;
    if (!queue->PutButtons(frame, buttons)) {
      // Error already logged.
      return PutButtonsStatus::REJECTED_BY_QUEUE;
    }

    // If this port is local, attach to the outgoing event for transmission.
    if (local_ports_.find(port) != local_ports_.end()) {
      VLOG(3)
          << "Port " << Port_Name(port)
          << " is local, attaching it to an outgoing event for transmission.";

      const int delay = DelayFramesForPort(port);
      if (delay < 0) {
        LOG(ERROR) << "Invalid delay frames for port " << Port_Name(port)
                   << ": " << delay;
        return PutButtonsStatus::INTERNAL_ERROR;
      }

      KeyStatePB* key = event.add_key_press();
      key->set_console_id(console_id_);
      key->set_frame_number(frame + delay);
      key->set_port(port);

      if (!coder_.EncodeButtons(buttons, key)) {
        LOG(ERROR) << "Failed to encode buttons.";
        return PutButtonsStatus::FAILED_TO_ENCODE;
      }
    } else {
      VLOG(3) << "Port " << Port_Name(port)
              << " is remote, not transmitting it.";
    }
  }

  // TODO(alexgolec): Use an asynchronous write here
  if (!event.key_press().empty()) {
    VLOG(3) << "Sending key presses:\n" << event.DebugString();

    timings_->add_event()->set_key_state_sync_write_start(
        client_utils::now_nanos());
    bool success = SyncWrite(event);
    timings_->add_event()->set_key_state_sync_write_finish(
        client_utils::now_nanos());

    if (!success) {
      LOG(ERROR) << "Failed to write outgoing event: " << event.DebugString();
      return PutButtonsStatus::FAILED_TO_TRANSMIT_REMOTE;
    }
  } else {
    VLOG(3) << "Event has no key presses:\n" << event.DebugString();
  }

  return PutButtonsStatus::SUCCESS;
}

// -----------------------------------------------------------------------------
// GetButtons and helpers

template <typename ButtonsType>
typename EventStreamHandler<ButtonsType>::GetButtonsStatus
EventStreamHandler<ButtonsType>::GetButtons(const Port port, int frame,
                                            ButtonsType* buttons) {
  if (local_ports_.find(port) != local_ports_.end()) {
    return GetLocalButtons(port, frame, buttons);
  } else {
    timings_->add_event()->set_remote_key_state_requested(
        client_utils::now_nanos());
    EventStreamHandler<ButtonsType>::GetButtonsStatus
        get_remote_buttons_status = GetRemoteButtons(port, frame, buttons);
    timings_->add_event()->set_remote_key_state_returned(
        client_utils::now_nanos());

    return get_remote_buttons_status;
  }
}

template <typename ButtonsType>
typename EventStreamHandler<ButtonsType>::GetButtonsStatus
EventStreamHandler<ButtonsType>::GetLocalButtons(const Port port, int frame,
                                                 ButtonsType* buttons) {
  ButtonsInputQueue* queue = GetQueue(port);
  if (queue == nullptr) {
    // Error already logged
    return GetButtonsStatus::NO_SUCH_PORT;
  }

  // If button data is already in the queue, return it immediately.
  typename ButtonsInputQueue::GetButtonsStatus status =
      queue->GetButtons(frame, 0 /* zero seconds */, buttons);

  if (status == ButtonsInputQueue::GetButtonsStatus::SUCCESS) {
    return GetButtonsStatus::SUCCESS;
  } else {
    LOG(ERROR) << "Failed to read buttons from local port " << Port_Name(port)
               << " and frame " << frame;
    return GetButtonsStatus::FAILURE;
  }
}

template <typename ButtonsType>
typename EventStreamHandler<ButtonsType>::GetButtonsStatus
EventStreamHandler<ButtonsType>::GetRemoteButtons(const Port port, int frame,
                                                  ButtonsType* buttons) {
  ButtonsInputQueue* queue = GetQueue(port);
  if (queue == nullptr) {
    // Error already logged
    return GetButtonsStatus::NO_SUCH_PORT;
  }

  // If button data is already in the queue, return it immediately.
  typename ButtonsInputQueue::GetButtonsStatus status =
      queue->GetButtons(frame, 0 /* zero seconds */, buttons);
  if (status == ButtonsInputQueue::GetButtonsStatus::SUCCESS) {
    return GetButtonsStatus::SUCCESS;
  } else if (status != ButtonsInputQueue::GetButtonsStatus::TIMEOUT) {
    LOG(ERROR) << "Failed to read buttons from remote port " << Port_Name(port)
               << " and frame " << frame;
    return GetButtonsStatus::FAILURE;
  }

  // Process the stream until we encounter the requested buttons.
  if (ReadUntilButtons(port, frame) != ReadUntilButtonsStatus::GOT_BUTTONS) {
    // TODO(alexgolec): Flesh these return statuses out a little instead of
    // returning failure on any non-button event.
    return GetButtonsStatus::FAILURE;
  }

  if (queue->GetButtons(frame, 5E6 /* five seconds */, buttons) !=
      ButtonsInputQueue::GetButtonsStatus::SUCCESS) {
    LOG(ERROR) << "Failed to get buttons for remote port " << Port_Name(port)
               << " and frame " << frame
               << " after reading the buttons event and placing them into the "
                  "queue. Probable logic error.";
    return GetButtonsStatus::FAILURE;
  }

  return GetButtonsStatus::SUCCESS;
}

template <typename ButtonsType>
typename EventStreamHandler<ButtonsType>::ReadUntilButtonsStatus
EventStreamHandler<ButtonsType>::ReadUntilButtons(const Port port, int frame) {
  bool found_buttons = false;

  while (!found_buttons) {
    IncomingEventPB event;

    VLOG(3) << "Looping on buttons for port " << Port_Name(port)
            << " and frame " << frame;

    timings_->add_event()->set_key_state_read_start(client_utils::now_nanos());
    bool success = SyncRead(&event);
    timings_->add_event()->set_key_state_read_finish(client_utils::now_nanos());
    if (!success) {
      LOG(ERROR) << "Failed to read event.";
      return ReadUntilButtonsStatus::RPC_READ_FAILURE;
    }

    VLOG(3) << "Read incoming event from stream:\n" << event.DebugString();

    if (event.has_stop_console()) {
      VLOG(3) << "Received console stopped message";
      return ReadUntilButtonsStatus::CONSOLE_TERMINATED;
    }

    // Return an error on all non-button statuses.
    // TODO(alexgolec): handle this more gracefully
    if (event.has_start_game() || !event.invalid_data().empty()) {
      LOG(ERROR)
          << "Received non-button message when expecting button message: "
          << event.DebugString();
      return ReadUntilButtonsStatus::NON_BUTTON_MESSAGE;
    }

    for (const KeyStatePB& keys : event.key_press()) {
      ButtonsInputQueue* queue = GetQueue(keys.port());
      if (queue == nullptr) {
        LOG(ERROR) << "Received buttons state data for unconnected port : "
                   << Port_Name(keys.port());
        return ReadUntilButtonsStatus::INVALID_BUTTONS_MESSAGE;
      }

      ButtonsType buttons;
      if (!coder_.DecodeButtons(keys, &buttons)) {
        LOG(ERROR) << "Failed to decode buttons from message: "
                   << keys.DebugString();
        return ReadUntilButtonsStatus::INVALID_BUTTONS_MESSAGE;
      }

      if (!queue->PutButtons(keys.frame_number(), buttons)) {
        LOG(ERROR) << "Failed to insert buttons into queue for port "
                   << Port_Name(keys.port()) << " and frame "
                   << keys.frame_number();
        return ReadUntilButtonsStatus::REJECTED_BY_QUEUE;
      }

      if (!found_buttons && keys.port() == port &&
          keys.frame_number() == frame) {
        VLOG(3) << "Found buttons for frame " << port << " and frame " << frame;
        found_buttons = true;
      }
    }
  }

  // We only exit the loop if we've found the requested frames.
  return ReadUntilButtonsStatus::GOT_BUTTONS;
}

// -----------------------------------------------------------------------------
// Close

template <typename ButtonsType>
void EventStreamHandler<ButtonsType>::TryCancel() {
  stream_context_.TryCancel();
}

// -----------------------------------------------------------------------------
// Accessors

template <typename ButtonsType>
std::set<Port> EventStreamHandler<ButtonsType>::local_ports() const {
  return local_ports_;
}

template <typename ButtonsType>
std::set<Port> EventStreamHandler<ButtonsType>::remote_ports() const {
  std::set<Port> ports;
  for (const auto& it : input_queues_) {
    const Port p = static_cast<Port>(it.first);
    if (local_ports_.find(p) == local_ports_.end()) {
      ports.insert(p);
    }
  }
  return ports;
}

template <typename ButtonsType>
int EventStreamHandler<ButtonsType>::DelayFramesForPort(Port port) const {
  const auto it = input_queues_.find(static_cast<int>(port));
  if (it == input_queues_.end()) {
    return -1;
  }
  return it->second->delay_frames();
}

// -----------------------------------------------------------------------------
// Utility methods

template <typename ButtonsType>
typename EventStreamHandler<ButtonsType>::ButtonsInputQueue*
EventStreamHandler<ButtonsType>::GetQueue(const Port port) {
  const auto it = input_queues_.find(port);
  if (it == input_queues_.end()) {
    LOG(ERROR) << "Requested queue for disconnected port: " << Port_Name(port);
    return nullptr;
  } else {
    return it->second.get();
  }
}

template <typename ButtonsType>
bool EventStreamHandler<ButtonsType>::SyncWrite(const OutgoingEventPB& event) {
  void* write_tag = reinterpret_cast<void*>(++stream_op_num_);
  stream_->Write(event, write_tag);
  return ExpectTag(cq_, write_tag);
}

template <typename ButtonsType>
bool EventStreamHandler<ButtonsType>::SyncRead(IncomingEventPB* event) {
  void* read_tag = reinterpret_cast<void*>(++stream_op_num_);
  stream_->Read(event, read_tag);
  return ExpectTag(cq_, read_tag);
}
