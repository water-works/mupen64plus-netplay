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
class MockClientAsyncReaderWriter
    : public grpc::ClientAsyncReaderWriterInterface<OutPB, InPB> {
 public:
  MOCK_METHOD1_T(ReadInitialMetadata, void(void*));
  MOCK_METHOD2_T(Finish, void(grpc::Status*, void*));

  MOCK_METHOD2_T(Write, void(const OutPB &, void *));
  MOCK_METHOD2_T(Read, void(IncomingEventPB *, void *));

  MOCK_METHOD1_T(WritesDone, void(void *));
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
  MOCK_METHOD2_T(PlugControllers,
                 bool(const std::vector<Port> &ports,
                      PlugControllerResponsePB::Status *status));
  MOCK_METHOD0_T(MakeEventStreamHandlerRaw,
                 EventStreamHandlerInterface<ButtonsType>*());
  MOCK_CONST_METHOD0_T(delay_frames, int());
  MOCK_CONST_METHOD0_T(console_id, int64_t());
  MOCK_CONST_METHOD0_T(client_id, int64_t());
  MOCK_CONST_METHOD0_T(local_ports, const std::vector<Port> &());
  MOCK_METHOD0(mutable_timings, TimingsPB*());
};

template <typename ButtonsType>
class MockEventStreamHandler : public EventStreamHandlerInterface<ButtonsType> {
 public:
  typedef EventStreamHandlerInterface<ButtonsType> BaseType;

  MOCK_CONST_METHOD0_T(status, typename BaseType::HandlerStatus());
  MOCK_METHOD0_T(ClientReady, void*());
  MOCK_METHOD1_T(WaitForConsoleStart, bool(void*));
  MOCK_METHOD3_T(GetButtons,
                 typename BaseType::GetButtonsStatus(const Port port, int frame,
                                                     ButtonsType *buttons));
  MOCK_METHOD1_T(PutButtons,
                 typename BaseType::PutButtonsStatus(const std::vector<
                     typename BaseType::ButtonsFrameTuple> &buttons_tuples));
  MOCK_METHOD0_T(TryCancel, void());
  MOCK_CONST_METHOD0_T(local_ports, std::set<Port>());
  MOCK_CONST_METHOD0_T(remote_ports, std::set<Port>());
  MOCK_CONST_METHOD1_T(DelayFramesForPort, int(Port port));
  MOCK_METHOD0_T(mutable_timings, TimingsPB *());
};

class MockCompletionQueueWrapper : public CompletionQueueWrapper {
 public:
  MOCK_METHOD2(Next, void(void **, bool *));
  void NextOk(uint64_t tag) {
    EXPECT_CALL(*this, Next(testing::_, testing::_))
        .WillOnce(testing::DoAll(
            testing::SetArgPointee<0>(reinterpret_cast<void *>(tag)),
            testing::SetArgPointee<1>(true)));
  }
};

#endif  // MOCKS_H_
