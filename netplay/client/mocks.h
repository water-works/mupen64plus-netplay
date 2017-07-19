#ifndef MOCKS_H_
#define MOCKS_H_

#include <vector>

#include "client/button-coder-interface.h"
#include "client/client.h"
#include "client/event-stream-handler.h"
#include "gmock/gmock.h"
#include "grpc++/support/sync_stream.h"
#include "base/netplayServiceProto.grpc.pb.h"
#include "base/netplayServiceProto.pb.h"

class MockNetPlayServerServiceStub
    : public NetPlayServerService::StubInterface {
 public:
  MOCK_METHOD3(Ping,
	       ::grpc::Status(::grpc::ClientContext *context,
			      const ::PingPB &request, ::PingPB *response));
  MOCK_METHOD3(MakeConsole,
	       ::grpc::Status(::grpc::ClientContext *context,
			      const ::MakeConsoleRequestPB &request,
			      ::MakeConsoleResponsePB *response));
  MOCK_METHOD3(QueryConsole,
           ::grpc::Status(::grpc::ClientContext *context,
                  const ::QueryConsoleRequestPB &request,
                  ::QueryConsoleResponsePB *response));
  MOCK_METHOD3(PlugController,
	       ::grpc::Status(::grpc::ClientContext *context,
			      const ::PlugControllerRequestPB &request,
			      ::PlugControllerResponsePB *response));
  MOCK_METHOD3(StartGame, ::grpc::Status(::grpc::ClientContext *context,
					 const ::StartGameRequestPB &request,
					 ::StartGameResponsePB *response));
  MOCK_METHOD3(ShutDownServer,
	       ::grpc::Status(::grpc::ClientContext *context,
			      const ::ShutDownServerRequestPB &request,
			      ::ShutDownServerResponsePB *response));

  // Implementations of private methods that return raw pointers instead of
  // unique_ptrs.
  MOCK_METHOD3(AsyncPingRaw,
	       ::grpc::ClientAsyncResponseReaderInterface<::PingPB> *(
		   ::grpc::ClientContext *context, const ::PingPB &request,
		   ::grpc::CompletionQueue *cq));
  MOCK_METHOD3(AsyncQueryConsoleRaw,
           ::grpc::ClientAsyncResponseReaderInterface<::QueryConsoleResponsePB> *(
           ::grpc::ClientContext *context, const ::QueryConsoleRequestPB &request,
           ::grpc::CompletionQueue *cq));
  MOCK_METHOD3(
      AsyncMakeConsoleRaw,
      ::grpc::ClientAsyncResponseReaderInterface<::MakeConsoleResponsePB> *(
	  ::grpc::ClientContext *context, const ::MakeConsoleRequestPB &request,
	  ::grpc::CompletionQueue *cq));
  MOCK_METHOD3(
      AsyncPlugControllerRaw,
      ::grpc::ClientAsyncResponseReaderInterface<::PlugControllerResponsePB> *(
	  ::grpc::ClientContext *context,
	  const ::PlugControllerRequestPB &request,
	  ::grpc::CompletionQueue *cq));
  MOCK_METHOD3(AsyncStartGameRaw,
	       ::grpc::ClientAsyncResponseReaderInterface<::StartGameResponsePB>
		   *(::grpc::ClientContext *context,
		     const ::StartGameRequestPB &request,
		     ::grpc::CompletionQueue *cq));
  MOCK_METHOD1(
      SendEventRaw,
      ::grpc::ClientReaderWriterInterface<::OutgoingEventPB, ::IncomingEventPB>
	  *(::grpc::ClientContext *context));
  MOCK_METHOD3(AsyncSendEventRaw,
	       ::grpc::ClientAsyncReaderWriterInterface<::OutgoingEventPB,
							::IncomingEventPB>
		   *(::grpc::ClientContext *context,
		     ::grpc::CompletionQueue *cq, void *tag));
  MOCK_METHOD3(
      AsyncShutDownServerRaw,
      ::grpc::ClientAsyncResponseReaderInterface<::ShutDownServerResponsePB> *(
	  ::grpc::ClientContext *context,
	  const ::ShutDownServerRequestPB &request, ::grpc::CompletionQueue *cq

	  ));
};

template <typename OutPB, typename InPB>
class MockClientReaderWriter
    : public grpc::ClientReaderWriterInterface<OutPB, InPB> {
 public:
  MOCK_METHOD0_T(WaitForInitialMetadata, void());
  MOCK_METHOD0_T(WritesDone, bool());
  MOCK_METHOD0_T(Finish, grpc::Status());
  MOCK_METHOD2_T(Write, bool(const OutPB &, const grpc::WriteOptions &));
  MOCK_METHOD1_T(Read, bool(InPB *));
  MOCK_METHOD1_T(NextMessageSize, bool(uint32_t*));
};

template <typename ButtonsType>
class MockButtonCoder : public ButtonCoderInterface<ButtonsType> {
 public:
  MOCK_CONST_METHOD2_T(EncodeButtons, bool(const ButtonsType &buttons_in,
					   KeyStatePB *buttons_out));
  MOCK_CONST_METHOD2_T(DecodeButtons, bool(const KeyStatePB &buttons_in,
					   ButtonsType *buttons_out));
};

template <typename ButtonsType>
class MockNetplayClient : public NetplayClientInterface<ButtonsType> {
 public:
  MOCK_METHOD4_T(PlugControllers,
                 bool(int64_t console_id, const std::string &rom_md5,
                      const std::vector<Port> &ports,
                      PlugControllerResponsePB::Status *status));
  MOCK_METHOD0_T(MakeEventStreamHandlerRaw,
		 EventStreamHandlerInterface<ButtonsType> *());
  MOCK_CONST_METHOD0_T(delay_frames, int());
  MOCK_CONST_METHOD0_T(console_id, int64_t());
  MOCK_CONST_METHOD0_T(client_id, int64_t());
  MOCK_CONST_METHOD0_T(local_ports, const std::vector<Port> &());
  MOCK_CONST_METHOD0_T(stub,
		       std::shared_ptr<NetPlayServerService::StubInterface>());
  MOCK_METHOD0(mutable_timings, TimingsPB *());
};

template <typename ButtonsType>
class MockEventStreamHandler : public EventStreamHandlerInterface<ButtonsType> {
 public:
  typedef EventStreamHandlerInterface<ButtonsType> BaseType;

  MOCK_CONST_METHOD0_T(status, typename BaseType::HandlerStatus());
  MOCK_METHOD0_T(ClientReady, bool());
  MOCK_METHOD0_T(WaitForConsoleStart, bool());
  MOCK_METHOD3_T(GetButtons,
		 typename BaseType::GetButtonsStatus(const Port port, int frame,
						     ButtonsType *buttons));
  MOCK_METHOD1_T(PutButtons,
		 typename BaseType::PutButtonsStatus(
		     const std::vector<typename BaseType::ButtonsFrameTuple>
			 &buttons_tuples));
  MOCK_METHOD0_T(TryCancel, void());
  MOCK_CONST_METHOD0_T(local_ports, std::set<Port>());
  MOCK_CONST_METHOD0_T(remote_ports, std::set<Port>());
  MOCK_CONST_METHOD1_T(DelayFramesForPort, int(Port port));
  MOCK_METHOD0_T(mutable_timings, TimingsPB *());
};

#endif  // MOCKS_H_
