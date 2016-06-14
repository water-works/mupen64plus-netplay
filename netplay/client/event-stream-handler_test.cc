#include "client/event-stream-handler.h"

#include <memory>
#include <string>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "base/timings.pb.h"
#include "client/mocks.h"

using std::string;

using testing::_;
using testing::AtMost;
using testing::ElementsAre;
using testing::InSequence;
using testing::Return;
using testing::Property;
using testing::SetArgPointee;
using testing::UnorderedElementsAre;

class EventStreamHandlerTest : public ::testing::Test {
 protected:
  typedef EventStreamHandler<string> StringHandler;
  typedef MockClientAsyncReaderWriter<OutgoingEventPB, IncomingEventPB>
      MockStream;

  EventStreamHandlerTest()
      : mock_stub_(new MockNetPlayServerServiceStub()),
        mock_cq_(new MockCompletionQueueWrapper()),
        mock_stream_(new MockStream()),
        handler_(new StringHandler(
            kConsoleId, kClientId, {PORT_1}, &timings_, &mock_coder_,
            std::unique_ptr<CompletionQueueWrapper>(mock_cq_),
            std::shared_ptr<MockNetPlayServerServiceStub>(mock_stub_))) {
    auto* start_game = start_game_event_.mutable_start_game();
    start_game->set_console_id(kConsoleId);

    // PORT_1 is local
    auto* connected_port = start_game->add_connected_ports();
    connected_port->set_port(PORT_1);
    connected_port->set_delay_frames(2);

    // PORT_2 is remote
    connected_port = start_game->add_connected_ports();
    connected_port->set_port(PORT_2);
    connected_port->set_delay_frames(0);

    // PORT_3 is remote
    connected_port = start_game->add_connected_ports();
    connected_port->set_port(PORT_3);
    connected_port->set_delay_frames(0);
  }

  void StartGame() {
    EXPECT_CALL(*mock_stub_, AsyncSendEventRaw(_, _, _))
        .WillOnce(Return(mock_stream_));
    EXPECT_CALL(*mock_stream_, Write(_, _));
    EXPECT_CALL(*mock_stream_, Read(_, _))
        .WillOnce(SetArgPointee<0>(start_game_event_));

    {
      testing::InSequence seq;
      // After stream open
      mock_cq_->NextOk(0LL);
      // Reads and writes
      mock_cq_->NextOk(1LL);
      mock_cq_->NextOk(2LL);
    }

    handler_->ReadyAndWaitForConsoleStart();
  }

  static const int kConsoleId;
  static const int kClientId;

  IncomingEventPB start_game_event_;
  // These pointers are owned by handler_
  MockNetPlayServerServiceStub* mock_stub_;
  MockCompletionQueueWrapper* mock_cq_;
  MockStream* mock_stream_;

  MockButtonCoder<string> mock_coder_;
  std::unique_ptr<StringHandler> handler_;
  TimingsPB timings_;
};

const int EventStreamHandlerTest::kConsoleId = 101;
const int EventStreamHandlerTest::kClientId = 1001;

TEST_F(EventStreamHandlerTest, NewlyConstructedStatus) {
  EXPECT_EQ(StringHandler::HandlerStatus::NOT_YET_STARTED, handler_->status());
}

// -----------------------------------------------------------------------------
// Constructor tests

TEST_F(EventStreamHandlerTest, EventStreamHandlerInvalidConsoleId) {
  EXPECT_DEATH(StringHandler(-1, kClientId, {PORT_1}, &timings_, &mock_coder_,
                             std::unique_ptr<CompletionQueueWrapper>(mock_cq_),
                             std::unique_ptr<MockNetPlayServerServiceStub>(
                                 new MockNetPlayServerServiceStub())),
               "invalid console_id");

  EXPECT_DEATH(StringHandler(0, kClientId, {PORT_1}, &timings_, &mock_coder_,
                             std::unique_ptr<CompletionQueueWrapper>(mock_cq_),
                             std::unique_ptr<MockNetPlayServerServiceStub>(
                                 new MockNetPlayServerServiceStub())),
               "invalid console_id");
}

TEST_F(EventStreamHandlerTest, EventStreamHandlerInvalidClientId) {
  EXPECT_DEATH(StringHandler(kConsoleId, -1, {PORT_1}, &timings_, &mock_coder_,
                             std::unique_ptr<CompletionQueueWrapper>(mock_cq_),
                             std::unique_ptr<MockNetPlayServerServiceStub>(
                                 new MockNetPlayServerServiceStub())),
               "invalid client_id");

  EXPECT_DEATH(StringHandler(kConsoleId, 0, {PORT_1}, &timings_, &mock_coder_,
                             std::unique_ptr<CompletionQueueWrapper>(mock_cq_),
                             std::unique_ptr<MockNetPlayServerServiceStub>(
                                 new MockNetPlayServerServiceStub())),
               "invalid client_id");
}

TEST_F(EventStreamHandlerTest, EventStreamHandlerTooManyLocalPorts) {
  EXPECT_DEATH(StringHandler(kConsoleId, kClientId,
                             {PORT_1, PORT_2, PORT_3, PORT_4, PORT_ANY},
                             &timings_, &mock_coder_,
                             std::unique_ptr<CompletionQueueWrapper>(mock_cq_),
                             std::unique_ptr<MockNetPlayServerServiceStub>(
                                 new MockNetPlayServerServiceStub())),
               "local_ports has too many elements");
}

TEST_F(EventStreamHandlerTest, EventStreamHandlerDuplicateLocalPorts) {
  EXPECT_DEATH(
      StringHandler(kConsoleId, kClientId, {PORT_1, PORT_2, PORT_3, PORT_1},
                    &timings_, &mock_coder_,
                    std::unique_ptr<CompletionQueueWrapper>(mock_cq_),
                    std::unique_ptr<MockNetPlayServerServiceStub>(
                        new MockNetPlayServerServiceStub())),
      "local_ports contains duplicate values");
}

TEST_F(EventStreamHandlerTest, EventStreamHandlerPortAny) {
  EXPECT_DEATH(StringHandler(kConsoleId, kClientId, {PORT_1, PORT_2, PORT_ANY},
                             &timings_, &mock_coder_,
                             std::unique_ptr<CompletionQueueWrapper>(mock_cq_),
                             std::unique_ptr<MockNetPlayServerServiceStub>(
                                 new MockNetPlayServerServiceStub())),
               "local_ports contains PORT_ANY");
}

// -----------------------------------------------------------------------------
// ReadyAndWaitForConsoleStart Tests

TEST_F(EventStreamHandlerTest, ReadyAndWaitForConsoleStartSuccess) {
  // client ready message
  EXPECT_CALL(*mock_stub_, AsyncSendEventRaw(_, _, _))
      .WillOnce(Return(mock_stream_));
  EXPECT_CALL(*mock_stream_, Write(_, _));
  EXPECT_CALL(*mock_stream_, Read(_, _))
      .WillOnce(SetArgPointee<0>(start_game_event_));

  {
    testing::InSequence seq;
    // After stream open
    mock_cq_->NextOk(0LL);
    // Reads and writes
    mock_cq_->NextOk(1LL);
    mock_cq_->NextOk(2LL);
  }

  ASSERT_TRUE(handler_->ReadyAndWaitForConsoleStart());
  EXPECT_THAT(handler_->local_ports(), UnorderedElementsAre(PORT_1));
  EXPECT_THAT(handler_->remote_ports(), UnorderedElementsAre(PORT_2, PORT_3));

  EXPECT_EQ(4, timings_.event_size());
  EXPECT_GT(timings_.event(0).client_ready_sync_write_start(), 0);
  EXPECT_GT(timings_.event(1).client_ready_sync_write_finish(), 0);
  EXPECT_GT(timings_.event(2).start_game_event_read_start(), 0);
  EXPECT_GT(timings_.event(3).start_game_event_read_finish(), 0);
}

TEST_F(EventStreamHandlerTest, ReadyAndWaitForConsoleStartStreamOpenFailed) {
  // client ready message
  EXPECT_CALL(*mock_stub_, AsyncSendEventRaw(_, _, _))
      .WillOnce(Return(mock_stream_));
  // After stream open
  EXPECT_CALL(*mock_cq_, Next(_, _)).WillOnce(SetArgPointee<1>(false));

  ASSERT_FALSE(handler_->ReadyAndWaitForConsoleStart());
}

TEST_F(EventStreamHandlerTest, ReadyAndWaitForConsoleStartFailedToWriteReady) {
  // client ready message
  EXPECT_CALL(*mock_stub_, AsyncSendEventRaw(_, _, _))
      .WillOnce(Return(mock_stream_));

  EXPECT_CALL(*mock_stream_, Write(_, _));

  {
    testing::InSequence seq;
    // After stream open
    mock_cq_->NextOk(0LL);
    // Reads and writes
    EXPECT_CALL(*mock_cq_, Next(_, _)).WillOnce(SetArgPointee<1>(false));
  }

  ASSERT_FALSE(handler_->ReadyAndWaitForConsoleStart());
  EXPECT_THAT(handler_->local_ports(), UnorderedElementsAre(PORT_1));
  EXPECT_THAT(handler_->remote_ports(), UnorderedElementsAre());
}

TEST_F(EventStreamHandlerTest,
       ReadyAndWaitForConsoleStartFailedToReadStartGame) {
  // client ready message
  EXPECT_CALL(*mock_stub_, AsyncSendEventRaw(_, _, _))
      .WillOnce(Return(mock_stream_));
  EXPECT_CALL(*mock_stream_, Write(_, _));

  {
    testing::InSequence seq;
    // After stream open
    mock_cq_->NextOk(0LL);
    // Reads and writes
    mock_cq_->NextOk(1LL);
  }

  // expected ready message
  EXPECT_CALL(*mock_stream_, Read(_, _));
  EXPECT_CALL(*mock_cq_, Next(_, _)).WillOnce(SetArgPointee<1>(false));

  ASSERT_FALSE(handler_->ReadyAndWaitForConsoleStart());
  EXPECT_THAT(handler_->local_ports(), UnorderedElementsAre(PORT_1));
  EXPECT_THAT(handler_->remote_ports(), UnorderedElementsAre());
}

TEST_F(EventStreamHandlerTest, ReadyAndWaitForConsoleStartConsoleStopped) {
  // client ready message
  EXPECT_CALL(*mock_stub_, AsyncSendEventRaw(_, _, _))
      .WillOnce(Return(mock_stream_));
  EXPECT_CALL(*mock_stream_, Write(_, _));

  start_game_event_.mutable_stop_console()->set_console_id(kConsoleId);
  start_game_event_.mutable_stop_console()->set_stop_reason(
      StopConsolePB::STOP_REQUESTED_BY_CLIENT);
  EXPECT_CALL(*mock_stream_, Read(_, _))
      .WillOnce(SetArgPointee<0>(start_game_event_));

  {
    testing::InSequence seq;
    // After stream open
    mock_cq_->NextOk(0LL);
    // Reads and writes
    mock_cq_->NextOk(1LL);
    mock_cq_->NextOk(2LL);
  }

  ASSERT_FALSE(handler_->ReadyAndWaitForConsoleStart());
  EXPECT_THAT(handler_->local_ports(), ElementsAre(PORT_1));
  EXPECT_THAT(handler_->remote_ports(), ElementsAre());
}

TEST_F(EventStreamHandlerTest, ReadyAndWaitForConsoleStartMissingStartGame) {
  // client ready message
  EXPECT_CALL(*mock_stub_, AsyncSendEventRaw(_, _, _))
      .WillOnce(Return(mock_stream_));
  EXPECT_CALL(*mock_stream_, Write(_, _));

  start_game_event_.clear_start_game();
  EXPECT_CALL(*mock_stream_, Read(_, _))
      .WillOnce(SetArgPointee<0>(start_game_event_));

  {
    testing::InSequence seq;
    // After stream open
    mock_cq_->NextOk(0LL);
    // Reads and writes
    mock_cq_->NextOk(1LL);
    mock_cq_->NextOk(2LL);
  }

  ASSERT_FALSE(handler_->ReadyAndWaitForConsoleStart());
  EXPECT_THAT(handler_->local_ports(), ElementsAre(PORT_1));
  EXPECT_THAT(handler_->remote_ports(), ElementsAre());
}

TEST_F(EventStreamHandlerTest, ReadyAndWaitForConsoleStartInvalidConsoleId) {
  {
    auto* mock_stream = new MockStream();
    EXPECT_CALL(*mock_stub_, AsyncSendEventRaw(_, _, _))
        .WillOnce(Return(mock_stream));
    EXPECT_CALL(*mock_stream, Write(_, _));

    IncomingEventPB start_game_event_negative_console_id;
    start_game_event_negative_console_id.mutable_start_game()->set_console_id(
        -1);
    EXPECT_CALL(*mock_stream, Read(_, _))
        .WillOnce(SetArgPointee<0>(start_game_event_negative_console_id));

    {
      testing::InSequence seq;
      // After stream open
      mock_cq_->NextOk(0LL);
      // Reads and writes
      mock_cq_->NextOk(1LL);
      mock_cq_->NextOk(2LL);
    }

    ASSERT_FALSE(handler_->ReadyAndWaitForConsoleStart());
    EXPECT_THAT(handler_->local_ports(), ElementsAre(PORT_1));
    EXPECT_THAT(handler_->remote_ports(), ElementsAre());
  }
  {
    auto* mock_stream = new MockStream();
    EXPECT_CALL(*mock_stub_, AsyncSendEventRaw(_, _, _))
        .WillOnce(Return(mock_stream));
    EXPECT_CALL(*mock_stream, Write(_, _));

    IncomingEventPB start_game_event_zero_console_id;
    start_game_event_zero_console_id.mutable_start_game()->set_console_id(0);
    EXPECT_CALL(*mock_stream, Read(_, _))
        .WillOnce(SetArgPointee<0>(start_game_event_zero_console_id));

    {
      testing::InSequence seq;
      // After stream open
      mock_cq_->NextOk(0LL);
      // Reads and writes
      mock_cq_->NextOk(3LL);
      mock_cq_->NextOk(4LL);
    }

    ASSERT_FALSE(handler_->ReadyAndWaitForConsoleStart());
    EXPECT_THAT(handler_->local_ports(), ElementsAre(PORT_1));
    EXPECT_THAT(handler_->remote_ports(), ElementsAre());
  }
  {
    auto* mock_stream = new MockStream();
    EXPECT_CALL(*mock_stub_, AsyncSendEventRaw(_, _, _))
        .WillOnce(Return(mock_stream));
    EXPECT_CALL(*mock_stream, Write(_, _));

    IncomingEventPB start_game_event_unexpected_console_id;
    start_game_event_unexpected_console_id.mutable_start_game()->set_console_id(
        kConsoleId + 1);
    EXPECT_CALL(*mock_stream, Read(_, _))
        .WillOnce(SetArgPointee<0>(start_game_event_unexpected_console_id));

    {
      testing::InSequence seq;
      // After stream open
      mock_cq_->NextOk(0LL);
      // Reads and writes
      mock_cq_->NextOk(5LL);
      mock_cq_->NextOk(6LL);
    }

    ASSERT_FALSE(handler_->ReadyAndWaitForConsoleStart());
    EXPECT_THAT(handler_->local_ports(), ElementsAre(PORT_1));
    EXPECT_THAT(handler_->remote_ports(), ElementsAre());
  }
}

TEST_F(EventStreamHandlerTest, ReadyAndWaitForConsoleStartMissingLocalPort) {
  // client ready message
  EXPECT_CALL(*mock_stub_, AsyncSendEventRaw(_, _, _))
      .WillOnce(Return(mock_stream_));
  EXPECT_CALL(*mock_stream_, Write(_, _));

  start_game_event_.mutable_start_game()->clear_connected_ports();
  EXPECT_CALL(*mock_stream_, Read(_, _))
      .WillOnce(SetArgPointee<0>(start_game_event_));

  {
    testing::InSequence seq;
    // After stream open
    mock_cq_->NextOk(0LL);
    // Reads and writes
    mock_cq_->NextOk(1LL);
    mock_cq_->NextOk(2LL);
  }

  ASSERT_FALSE(handler_->ReadyAndWaitForConsoleStart());
  EXPECT_THAT(handler_->local_ports(), ElementsAre(PORT_1));
  EXPECT_THAT(handler_->remote_ports(), ElementsAre());
}

TEST_F(EventStreamHandlerTest, ReadyAndWaitForConsoleStartDuplicatePort) {
  // client ready message
  EXPECT_CALL(*mock_stub_, AsyncSendEventRaw(_, _, _))
      .WillOnce(Return(mock_stream_));
  EXPECT_CALL(*mock_stream_, Write(_, _));

  auto* connected_port =
      start_game_event_.mutable_start_game()->add_connected_ports();
  connected_port->set_port(PORT_1);
  connected_port->set_delay_frames(2);
  EXPECT_CALL(*mock_stream_, Read(_, _))
      .WillOnce(SetArgPointee<0>(start_game_event_));

  {
    testing::InSequence seq;
    // After stream open
    mock_cq_->NextOk(0LL);
    // Reads and writes
    mock_cq_->NextOk(1LL);
    mock_cq_->NextOk(2LL);
  }

  ASSERT_FALSE(handler_->ReadyAndWaitForConsoleStart());
  EXPECT_THAT(handler_->local_ports(), ElementsAre(PORT_1));
  EXPECT_THAT(handler_->remote_ports(), ElementsAre());
}

TEST_F(EventStreamHandlerTest, ReadyAndWaitForConsoleStartTooManyPorts) {
  // client ready message
  EXPECT_CALL(*mock_stub_, AsyncSendEventRaw(_, _, _))
      .WillOnce(Return(mock_stream_));
  EXPECT_CALL(*mock_stream_, Write(_, _));

  auto* connected_port =
      start_game_event_.mutable_start_game()->add_connected_ports();
  connected_port->set_port(PORT_3);
  connected_port->set_delay_frames(2);

  connected_port =
      start_game_event_.mutable_start_game()->add_connected_ports();
  connected_port->set_port(PORT_4);
  connected_port->set_delay_frames(2);

  connected_port =
      start_game_event_.mutable_start_game()->add_connected_ports();
  connected_port->set_port(PORT_ANY);
  connected_port->set_delay_frames(2);

  EXPECT_CALL(*mock_stream_, Read(_, _))
      .WillOnce(SetArgPointee<0>(start_game_event_));

  {
    testing::InSequence seq;
    // After stream open
    mock_cq_->NextOk(0LL);
    // Reads and writes
    mock_cq_->NextOk(1LL);
    mock_cq_->NextOk(2LL);
  }

  ASSERT_FALSE(handler_->ReadyAndWaitForConsoleStart());
  EXPECT_THAT(handler_->local_ports(), ElementsAre(PORT_1));
  EXPECT_THAT(handler_->remote_ports(), ElementsAre());
}

TEST_F(EventStreamHandlerTest, ReadyAndWaitForConsoleStartPortAny) {
  // client ready message
  EXPECT_CALL(*mock_stub_, AsyncSendEventRaw(_, _, _))
      .WillOnce(Return(mock_stream_));
  EXPECT_CALL(*mock_stream_, Write(_, _));

  auto* connected_port =
      start_game_event_.mutable_start_game()->add_connected_ports();
  connected_port->set_port(PORT_ANY);
  connected_port->set_delay_frames(2);

  EXPECT_CALL(*mock_stream_, Read(_, _))
      .WillOnce(SetArgPointee<0>(start_game_event_));

  {
    testing::InSequence seq;
    // After stream open
    mock_cq_->NextOk(0LL);
    // Reads and writes
    mock_cq_->NextOk(1LL);
    mock_cq_->NextOk(2LL);
  }

  ASSERT_FALSE(handler_->ReadyAndWaitForConsoleStart());
  EXPECT_THAT(handler_->local_ports(), ElementsAre(PORT_1));
  EXPECT_THAT(handler_->remote_ports(), ElementsAre());
}

TEST_F(EventStreamHandlerTest,
       ReadyAndWaitForConsoleNotAllLocalPortsConnected) {
  // client ready message
  EXPECT_CALL(*mock_stream_, Write(_, _));

  // PORT_2
  auto first_port = start_game_event_.mutable_start_game()->connected_ports(1);
  start_game_event_.mutable_start_game()->clear_connected_ports();
  *start_game_event_.mutable_start_game()->add_connected_ports() = first_port;

  EXPECT_CALL(*mock_stream_, Read(_, _))
      .WillOnce(SetArgPointee<0>(start_game_event_));

  {
    testing::InSequence seq;
    mock_cq_->NextOk(1LL);
    mock_cq_->NextOk(2LL);
  }

  ASSERT_FALSE(handler_->ReadyAndWaitForConsoleStart());
  EXPECT_THAT(handler_->local_ports(), ElementsAre(PORT_1));
  EXPECT_THAT(handler_->remote_ports(), ElementsAre());
}

// -----------------------------------------------------------------------------
// PutButtons

TEST_F(EventStreamHandlerTest, PutButtonsEmptyVector) {
  StartGame();

  EXPECT_EQ(StringHandler::PutButtonsStatus::SUCCESS, handler_->PutButtons({}));
}

TEST_F(EventStreamHandlerTest, PutButtonsOnlyTransmitLocalPorts) {
  StartGame();

  const string port_1_data = "PORT_1 frame 0";
  EXPECT_CALL(mock_coder_, EncodeButtons(port_1_data, _))
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_stream_, Write(_, _));
  mock_cq_->NextOk(3LL);

  EXPECT_EQ(
      StringHandler::PutButtonsStatus::SUCCESS,
      handler_->PutButtons({std::make_tuple(PORT_1, 0, port_1_data),
                            std::make_tuple(PORT_2, 0, "PORT 2 frame 0"),
                            std::make_tuple(PORT_3, 0, "PORT 3 frame 0")}));
  // This test will fail it mock_coder_.EncoderButons and mock_stream_->Write
  // are called.

  // Events 1-4 were added through StartGame().
  EXPECT_EQ(6, timings_.event_size());
  EXPECT_GT(timings_.event(4).key_state_sync_write_start(), 0);
  EXPECT_GT(timings_.event(5).key_state_sync_write_finish(), 0);
}

TEST_F(EventStreamHandlerTest, PutButtonsDisconnectedPort) {
  StartGame();

  EXPECT_CALL(mock_coder_, EncodeButtons(_, _))
      .Times(AtMost(1))
      .WillOnce(Return(true));

  EXPECT_EQ(StringHandler::PutButtonsStatus::NO_SUCH_PORT,
            handler_->PutButtons(
                {std::make_tuple(PORT_1, 0, "PORT_1 frame 0"),
                 std::make_tuple(PORT_4, 0, "PORT_4 is disconnected")}));
}

TEST_F(EventStreamHandlerTest, PutButtonsUnexpectedFrameNum) {
  StartGame();

  EXPECT_CALL(mock_coder_, EncodeButtons(_, _))
      .Times(AtMost(1))
      .WillOnce(Return(true));

  EXPECT_EQ(
      StringHandler::PutButtonsStatus::REJECTED_BY_QUEUE,
      handler_->PutButtons({std::make_tuple(PORT_1, 0, "PORT_1 frame 0"),
                            std::make_tuple(PORT_2, -1, "unexpected frame")}));
}

TEST_F(EventStreamHandlerTest, EncodeFailure) {
  StartGame();

  EXPECT_CALL(mock_coder_, EncodeButtons(_, _))
      .Times(AtMost(1))
      .WillOnce(Return(false));

  EXPECT_EQ(
      StringHandler::PutButtonsStatus::FAILED_TO_ENCODE,
      handler_->PutButtons({std::make_tuple(PORT_1, 0, "PORT_1 frame 0")}));
}

TEST_F(EventStreamHandlerTest, WriteFailure) {
  StartGame();

  const string data("PORT_1 frame 0");
  EXPECT_CALL(mock_coder_, EncodeButtons(data, _))
      .Times(AtMost(1))
      .WillOnce(Return(true));

  EXPECT_CALL(*mock_stream_, Write(_, _));
  EXPECT_CALL(*mock_cq_, Next(_, _)).WillOnce(SetArgPointee<1>(false));

  EXPECT_EQ(StringHandler::PutButtonsStatus::FAILED_TO_TRANSMIT_REMOTE,
            handler_->PutButtons({std::make_tuple(PORT_1, 0, data)}));
}

// -----------------------------------------------------------------------------
// GetButtons

TEST_F(EventStreamHandlerTest, GetButtonsInvalidPort) {
  StartGame();

  // PORT_4 is not allocated
  string data;
  EXPECT_EQ(StringHandler::GetButtonsStatus::NO_SUCH_PORT,
            handler_->GetButtons(PORT_4, 0, &data));
}

TEST_F(EventStreamHandlerTest, GetButtonsLocalPortButtonsNotInQueue) {
  StartGame();

  string data;

  // Extract the delay frames first
  EXPECT_EQ(StringHandler::GetButtonsStatus::SUCCESS,
            handler_->GetButtons(PORT_1, 0, &data));
  EXPECT_EQ(StringHandler::GetButtonsStatus::SUCCESS,
            handler_->GetButtons(PORT_1, 1, &data));

  EXPECT_EQ(StringHandler::GetButtonsStatus::FAILURE,
            handler_->GetButtons(PORT_1, 2, &data));
}

TEST_F(EventStreamHandlerTest, GetButtonsInvalidFrame) {
  StartGame();

  string data;
  // Local port.
  EXPECT_EQ(StringHandler::GetButtonsStatus::FAILURE,
            handler_->GetButtons(PORT_1, -1, &data));
  // Remote port
  EXPECT_EQ(StringHandler::GetButtonsStatus::FAILURE,
            handler_->GetButtons(PORT_3, -1, &data));
}

TEST_F(EventStreamHandlerTest, GetButtonsLocalPort) {
  StartGame();

  EXPECT_CALL(mock_coder_, EncodeButtons(_, _)).WillOnce(Return(true));
  EXPECT_CALL(*mock_stream_, Write(_, _));
  mock_cq_->NextOk(3LL);

  ASSERT_EQ(StringHandler::PutButtonsStatus::SUCCESS,
            handler_->PutButtons({std::make_tuple(PORT_1, 0, "frame 0")}));

  string data;

  // Extract the delay frames first.
  ASSERT_EQ(StringHandler::GetButtonsStatus::SUCCESS,
            handler_->GetButtons(PORT_1, 0, &data));
  EXPECT_EQ("", data);
  ASSERT_EQ(StringHandler::GetButtonsStatus::SUCCESS,
            handler_->GetButtons(PORT_1, 1, &data));
  EXPECT_EQ("", data);

  ASSERT_EQ(StringHandler::GetButtonsStatus::SUCCESS,
            handler_->GetButtons(PORT_1, 2, &data));
  EXPECT_EQ("frame 0", data);
}

TEST_F(EventStreamHandlerTest,
       GetButtonsRemotePortReadFromStreamMultipleMessages) {
  StartGame();

  {
    InSequence sequence;

    // PORT_3 comes over the wire first
    IncomingEventPB event_3;
    KeyStatePB* keys_3 = event_3.add_key_press();
    keys_3->set_console_id(kConsoleId);
    keys_3->set_port(PORT_3);
    keys_3->set_frame_number(0);
    keys_3->set_reserved_1(300);

    EXPECT_CALL(*mock_stream_, Read(_, _)).WillOnce(SetArgPointee<0>(event_3));
    EXPECT_CALL(mock_coder_,
                DecodeButtons(Property(&KeyStatePB::reserved_1, 300), _))
        .WillOnce(DoAll(SetArgPointee<1>("data 300"), Return(true)));

    // PORT_2 comes over the wire next
    IncomingEventPB event_2;
    KeyStatePB* keys_2 = event_2.add_key_press();
    keys_2->set_console_id(kConsoleId);
    keys_2->set_port(PORT_2);
    keys_2->set_frame_number(0);
    keys_2->set_reserved_1(200);

    EXPECT_CALL(*mock_stream_, Read(_, _)).WillOnce(SetArgPointee<0>(event_2));
    EXPECT_CALL(mock_coder_,
                DecodeButtons(Property(&KeyStatePB::reserved_1, 200), _))
        .WillOnce(DoAll(SetArgPointee<1>("data 200"), Return(true)));
  }

  string data;
  ASSERT_EQ(StringHandler::GetButtonsStatus::SUCCESS,
            handler_->GetButtons(PORT_2, 0, &data));
  EXPECT_EQ("data 200", data);

  data = "";
  ASSERT_EQ(StringHandler::GetButtonsStatus::SUCCESS,
            handler_->GetButtons(PORT_3, 0, &data));
  EXPECT_EQ("data 300", data);
}

TEST_F(EventStreamHandlerTest, GetButtonsRemotePortReadFromStreamOneMessage) {
  StartGame();

  {
    InSequence sequence;

    // PORT_3
    IncomingEventPB event;
    KeyStatePB* keys = event.add_key_press();
    keys->set_console_id(kConsoleId);
    keys->set_port(PORT_3);
    keys->set_frame_number(0);
    keys->set_reserved_1(300);

    // PORT_2
    keys = event.add_key_press();
    keys->set_console_id(kConsoleId);
    keys->set_port(PORT_2);
    keys->set_frame_number(0);
    keys->set_reserved_1(200);

    EXPECT_CALL(*mock_stream_, Read(_, _)).WillOnce(SetArgPointee<0>(event));
    mock_cq_->NextOk(3LL);

    EXPECT_CALL(mock_coder_,
                DecodeButtons(Property(&KeyStatePB::reserved_1, 300), _))
        .WillOnce(DoAll(SetArgPointee<1>("data 300"), Return(true)));
    EXPECT_CALL(mock_coder_,
                DecodeButtons(Property(&KeyStatePB::reserved_1, 200), _))
        .WillOnce(DoAll(SetArgPointee<1>("data 200"), Return(true)));
  }

  string data;
  ASSERT_EQ(StringHandler::GetButtonsStatus::SUCCESS,
            handler_->GetButtons(PORT_2, 0, &data));
  EXPECT_EQ("data 200", data);

  data = "";
  ASSERT_EQ(StringHandler::GetButtonsStatus::SUCCESS,
            handler_->GetButtons(PORT_3, 0, &data));
  EXPECT_EQ("data 300", data);

  // Events 1-4 were added through StartGame().
  EXPECT_EQ(10, timings_.event_size());
  EXPECT_GT(timings_.event(4).remote_key_state_requested(), 0);
  EXPECT_GT(timings_.event(5).key_state_read_start(), 0);
  EXPECT_GT(timings_.event(6).key_state_read_finish(), 0);
  EXPECT_GT(timings_.event(7).remote_key_state_returned(), 0);
  EXPECT_GT(timings_.event(8).remote_key_state_requested(), 0);
  EXPECT_GT(timings_.event(9).remote_key_state_returned(), 0);
}

TEST_F(EventStreamHandlerTest, GetButtonsRemotePortNonButtonMessage) {
  StartGame();

  // PORT_3
  IncomingEventPB event;
  KeyStatePB* keys = event.add_key_press();
  keys->set_console_id(kConsoleId);
  keys->set_port(PORT_3);
  keys->set_frame_number(0);
  keys->set_reserved_1(300);

  event.add_invalid_data();

  EXPECT_CALL(*mock_stream_, Read(_, _)).WillOnce(SetArgPointee<0>(event));
  mock_cq_->NextOk(3LL);

  string data;
  ASSERT_EQ(StringHandler::GetButtonsStatus::FAILURE,
            handler_->GetButtons(PORT_3, 0, &data));
}

TEST_F(EventStreamHandlerTest, GetButtonsRemotePortConsoleStopped) {
  StartGame();

  IncomingEventPB event;
  event.mutable_stop_console()->set_console_id(kConsoleId);
  event.mutable_stop_console()->set_stop_reason(StopConsolePB::ERROR);

  EXPECT_CALL(*mock_stream_, Read(_, _)).WillOnce(SetArgPointee<0>(event));
  mock_cq_->NextOk(3LL);

  string data;
  ASSERT_EQ(StringHandler::GetButtonsStatus::FAILURE,
            handler_->GetButtons(PORT_3, 0, &data));
}

TEST_F(EventStreamHandlerTest, GetButtonsRemotePortFailedToRead) {
  StartGame();

  EXPECT_CALL(*mock_stream_, Read(_, _));
  EXPECT_CALL(*mock_cq_, Next(_, _)).WillOnce(SetArgPointee<1>(false));

  string data;
  ASSERT_EQ(StringHandler::GetButtonsStatus::FAILURE,
            handler_->GetButtons(PORT_3, 0, &data));
}

TEST_F(EventStreamHandlerTest, GetButtonsRemotePortDisconnectedPort) {
  StartGame();

  // PORT_4
  IncomingEventPB event;
  KeyStatePB* keys = event.add_key_press();
  keys->set_console_id(kConsoleId);
  keys->set_port(PORT_4);
  keys->set_frame_number(0);

  EXPECT_CALL(*mock_stream_, Read(_, _)).WillOnce(SetArgPointee<0>(event));
  mock_cq_->NextOk(3LL);

  string data;
  ASSERT_EQ(StringHandler::GetButtonsStatus::FAILURE,
            handler_->GetButtons(PORT_3, 0, &data));
}

TEST_F(EventStreamHandlerTest, GetButtonsRemotePortDecodeFailure) {
  StartGame();

  // PORT_3
  IncomingEventPB event;
  KeyStatePB* keys = event.add_key_press();
  keys->set_console_id(kConsoleId);
  keys->set_port(PORT_3);
  keys->set_frame_number(0);
  keys->set_reserved_1(300);

  EXPECT_CALL(*mock_stream_, Read(_, _)).WillOnce(SetArgPointee<0>(event));
  mock_cq_->NextOk(3LL);

  EXPECT_CALL(mock_coder_,
              DecodeButtons(Property(&KeyStatePB::reserved_1, 300), _))
      .WillOnce(Return(false));

  string data;
  ASSERT_EQ(StringHandler::GetButtonsStatus::FAILURE,
            handler_->GetButtons(PORT_3, 0, &data));
}

TEST_F(EventStreamHandlerTest, GetButtonsRemotePortRejectedByQueue) {
  StartGame();

  // PORT_3
  IncomingEventPB event;
  KeyStatePB* keys = event.add_key_press();
  keys->set_console_id(kConsoleId);
  keys->set_port(PORT_3);
  keys->set_frame_number(-1);
  keys->set_reserved_1(300);

  EXPECT_CALL(*mock_stream_, Read(_, _)).WillOnce(SetArgPointee<0>(event));
  mock_cq_->NextOk(3LL);

  EXPECT_CALL(mock_coder_,
              DecodeButtons(Property(&KeyStatePB::reserved_1, 300), _))
      .WillOnce(Return(true));

  string data;
  ASSERT_EQ(StringHandler::GetButtonsStatus::FAILURE,
            handler_->GetButtons(PORT_3, 0, &data));
}
