#ifndef EVENT_STREAM_HANDLER_H_
#define EVENT_STREAM_HANDLER_H_

#include <memory>
#include <unordered_map>
#include <set>

#include <google/protobuf/repeated_field.h>

#include "base/netplayServiceProto.grpc.pb.h"
#include "base/timings.pb.h"
#include "client/button-coder-interface.h"
#include "client/input-queue.h"

// Thin, mockable wrapper around the completion queue.
class CompletionQueueWrapper {
 public:
  virtual ~CompletionQueueWrapper() {}
  virtual void Next(void** tag, bool* ok) { cq.Next(tag, ok); }
  // Note this will be accessed directly by the stream handler for both real and
  // mock instance of this class.
  grpc::CompletionQueue cq;
};

template <typename ButtonsType>
class EventStreamHandlerInterface {
 public:
  virtual ~EventStreamHandlerInterface() {}

  typedef std::tuple<Port,         // update port
                     int,          // frame number
                     ButtonsType>  // data
      ButtonsFrameTuple;

  // TODO(alexgolec): Migrate all these statuses to proto statuses and print 
  // their names in error messages instead of raw enum error codes.

  // Returns current status of the console.
  enum class HandlerStatus {
    // The console hasn't yet been started, or we haven't yet called
    // WaitForConsoleStart(). This is the default status for newly-initialized
    // handlers.
    NOT_YET_STARTED = 0,

    // The console was started and is forwarding events.
    CONSOLE_RUNNING,

    // The console was terminated and the event stream was closed.
    CONSOLE_TERMINATED
  };

  enum class GetButtonsStatus {
    SUCCESS = 0,
    // Attempted to get bottons from a port that is not connected.
    NO_SUCH_PORT,
    // Generic failure
    // TODO(alexgolec): split this into finer-grained statuses.
    FAILURE
  };

  enum class PutButtonsStatus {
    SUCCESS = 0,
    // Attempted to send buttons on a port that is not connected.
    NO_SUCH_PORT,
    // Indicates that the buttons could not be inserted into the queue for the
    // specified port.
    REJECTED_BY_QUEUE,
    // Failed to transmit remote port button data to the server.
    FAILED_TO_TRANSMIT_REMOTE,
    // Failed to encode the given buttons.
    FAILED_TO_ENCODE,
    // Internal logic error.
    INTERNAL_ERROR
  };

  virtual HandlerStatus status() const = 0;
  virtual bool ReadyAndWaitForConsoleStart() = 0;
  virtual GetButtonsStatus GetButtons(const Port port, int frame,
                                      ButtonsType* buttons) = 0;
  virtual PutButtonsStatus PutButtons(
      const std::vector<ButtonsFrameTuple>& buttons_tuples) = 0;
  virtual void TryCancel() = 0;
  virtual std::set<Port> local_ports() const = 0;
  virtual std::set<Port> remote_ports() const = 0;
  virtual int DelayFramesForPort(Port port) const = 0;
  virtual TimingsPB* mutable_timings() = 0;
};

// Handler that listens to a GRPC bidirectional stream and interprets the game
// events that pass back and forth.
template <typename ButtonsType>
class EventStreamHandler : public EventStreamHandlerInterface<ButtonsType> {
 public:
  typedef grpc::ClientAsyncReaderWriterInterface<
      OutgoingEventPB, IncomingEventPB> BidirectionalStream;
  typedef typename EventStreamHandlerInterface<ButtonsType>::HandlerStatus
      HandlerStatus;

  // Constructs a new stream handler. Arguments are:
  //  - console_id: console ID, std::abort's if nonpositive.
  //  - client_id: client ID, std::abort's if nonpositive
  //  - local_ports: locally attached ports. std::abort's if:
  //     - local_ports.size > 4
  //     - any ports repeat themselves
  //     - any port has value PORT_ANY
  //  - coder: pointer to a coder object used to encode and decode buttons to
  //    and from KeyStatePB protos.
  //  - stub: stub from which to produce a stream handle.
  EventStreamHandler(int console_id, int client_id,
                     const std::vector<Port> local_ports, TimingsPB* timings,
                     const ButtonCoderInterface<ButtonsType>* coder,
                     std::unique_ptr<CompletionQueueWrapper> cq,
                     std::shared_ptr<NetPlayServerService::StubInterface> stub);

  HandlerStatus status() const override {
    return status_;
  }

  // Signal to the server that we are ready to start the game and wait until
  // the server indicates the console has started.
  bool ReadyAndWaitForConsoleStart();

  // Send the given buttons to the server.
  typedef typename EventStreamHandlerInterface<ButtonsType>::PutButtonsStatus
      PutButtonsStatus;
  typedef typename EventStreamHandlerInterface<ButtonsType>::ButtonsFrameTuple
      ButtonsFrameTuple;
  PutButtonsStatus PutButtons(
      const std::vector<ButtonsFrameTuple>& buttons_frames) override;

  // Read the given buttons from the server. Blocks until the client has
  // received the buttons for the given frame from the server, or until a game
  // management event occurred.
  typedef typename EventStreamHandlerInterface<ButtonsType>::GetButtonsStatus
      GetButtonsStatus;
  GetButtonsStatus GetButtons(const Port port, int frame,
                              ButtonsType* buttons) override;

  // Abruptly cancel the stream. Note this method cannot guarantee the stream 
  // will actually be cancelled.
  // TODO(alexgolec): Implement a clean protocol-based method to end the game 
  // and close the stream.
  void TryCancel() override;

  // Returns the local ports with which this handler was constructed.
  std::set<Port> local_ports() const override;
  // Returns the remote ports allocated as part of ReadyAndWaitForConsoleStart. 
  // Note this method will return an empty container if
  // ReadyAndWaitForConsoleStart was not successfully called.
  std::set<Port> remote_ports() const override;

  // Returns the number of delay frames for the given port, or -1 if port is 
  // disconnected.
  int DelayFramesForPort(Port port) const override;

  // Return the timings with which this object was initialized.
  TimingsPB* mutable_timings() override { return timings_; };

 private:
  typedef InputQueue<ButtonsType> ButtonsInputQueue;

  // Parse the returned port configuration and initialize the queues for each
  // port.
  bool InitializeQueues(const google::protobuf::RepeatedPtrField<
      StartGamePB::ConnectedPortPB>& ports);

  // Get the buttons from a local port. Assumes that port is a local port.
  GetButtonsStatus GetLocalButtons(const Port port, int frame,
                                   ButtonsType* buttons);

  // Get the buttons for a remote port.
  GetButtonsStatus GetRemoteButtons(const Port port, int frame,
                                    ButtonsType* buttons);

  // Transmit the given buttons on the stream. std::abort's if the port is
  // invalid or local.
  bool SendButtons(const Port port, int frame, const ButtonsType& buttons);

  // Helper method to GetButtons that reads from stream_ until the buttons for
  // the given port number and frame arrive. Returns the following:
  //  - GOT_BUTTONS: If the requested frame data was received.
  //  - RPC_READ_FAILURE: If the read on stream_ returned false for any reason.
  // If this method returns with status GOT_BUTTONS, the queue associated with
  // the given port will have button data ready for the given frame.
  enum class ReadUntilButtonsStatus {
    GOT_BUTTONS = 0,
    RPC_READ_FAILURE,
    NON_BUTTON_MESSAGE,
    INVALID_BUTTONS_MESSAGE,
    REJECTED_BY_QUEUE,
    CONSOLE_TERMINATED
  };
  ReadUntilButtonsStatus ReadUntilButtons(const Port port, int frame);

  // Utility method that returns a borrowed pointer to a queue, or nullptr if  
  // there is no queue for the given port. Logs an error if there is no queue 
  // for the given port.
  ButtonsInputQueue* GetQueue(const Port port);

  // Emulate synchronous read and write operations on the async stream.
  uint64_t stream_op_num_ = 0;
  bool SyncWrite(const OutgoingEventPB& event);
  bool SyncRead(IncomingEventPB* event);

  const int console_id_;
  const int client_id_;
  std::set<Port> local_ports_;
  TimingsPB* timings_;
  // Borrowed reference
  const ButtonCoderInterface<ButtonsType>& coder_;
  std::shared_ptr<NetPlayServerService::StubInterface> stub_;
  std::unique_ptr<CompletionQueueWrapper> cq_;
  grpc::ClientContext stream_context_;
  std::unique_ptr<BidirectionalStream> stream_;

  std::unordered_map<int /* Port */, std::unique_ptr<ButtonsInputQueue>>
      input_queues_;

  HandlerStatus status_;
};

#include "event-stream-handler.hpp"

#endif  // EVENT_STREAM_HANDLER_H_
