#include <memory>
#include <thread>

#include "glog/logging.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "base/netplayServiceProto.grpc.pb.h"
#include "client/button-coder-interface.h"
#include "client/client.h"
#include "client/host-utils.h"
#include "integration_tests/integration-test-main.h"
#include "integration_tests/netplay-server-test-fixture.h"

using std::shared_ptr;
using std::thread;
using std::unique_ptr;

using testing::Contains;
using testing::UnorderedElementsAre;

class NetplayIntegrationTest : public testing::Test {
 public:
  typedef EventStreamHandlerInterface<uint32_t> StreamHandler;

  NetplayIntegrationTest() : fixture_(*integration_tests::GetJars()) {}

  void SetUp() override {
    integration_tests::MUST_USE_INTEGRATION_TEST_MAIN();

    LOG(INFO) << "Starting VM";
    fixture_.StartVM();
    if (!fixture_.WaitForPing(10, 500)) {
      LOG(INFO)
          << "Timed out waiting for the server to respond to a ping request.";
      exit(1);
    }

    p1_stub_ = fixture_.MakeStub();
    p2_stub_ = fixture_.MakeStub();
  }

  void TearDown() override {
    LOG(INFO) << "Shutting down Netplay server thread.";
    fixture_.ShutDownTestServer();
  }

  // Frame data is written as follows:
  //  - Bits 31-16: port as an integer
  //  - Bits 15-0 : frame_num
  uint32_t MakeFrameData(Port port, uint16_t frame_num) {
    return static_cast<uint16_t>(port) << 16 | frame_num;
  }

  bool SendFrame(Port port, int frame_num, StreamHandler* handler) {
    uint32_t frame_data = MakeFrameData(port, frame_num);
    StreamHandler::ButtonsFrameTuple frame_tuple(
        std::make_tuple(port, frame_num, frame_data));
    StreamHandler::PutButtonsStatus status = handler->PutButtons({frame_tuple});
    if (status != StreamHandler::PutButtonsStatus::SUCCESS) {
      LOG(ERROR) << "Failed to put buttons into port " << Port_Name(port)
                 << " for frame number " << frame_num;
      return false;
    }
    return true;
  }

  uint32_t GetFrame(Port port, int frame_num, StreamHandler* handler) {
    uint32_t frame_data;
    StreamHandler::GetButtonsStatus status =
        handler->GetButtons(port, frame_num, &frame_data);
    if (status != StreamHandler::GetButtonsStatus::SUCCESS) {
      LOG(ERROR) << "Failed to get buttons from port " << Port_Name(port)
                 << " for frame number " << frame_num;
      return 0;
    }
    return frame_data;
  }

  // Wait until the server declares that we're ready for a game.
  bool ReadyForGame(int num_attempts, int wait_per_attempt_millis,
                    StreamHandler* handler) {
    for (int i = 0; i < num_attempts; ++i) {
      if (handler->ReadyAndWaitForConsoleStart()) {
        return true;
      }

      LOG(ERROR) << "Handler ReadyAndWaitForConsoleStart failed. Retrying.";

      std::this_thread::sleep_for(
          std::chrono::milliseconds(wait_per_attempt_millis));
    }

    return false;
  }

  // Start the game using player 1. This method will loop for the specified
  // number of attempts until all the clients are connected and register
  // themselves as ready to read. Returns true if the game was successfully
  // started.
  bool StartGame(int console_id, int num_attempts, int rpc_timeout_millis,
                 int wait_per_attempt_millis) {
    for (int i = 0; i < num_attempts; ++i) {
      grpc::ClientContext context;
      context.set_deadline(std::chrono::system_clock::now() +
                           std::chrono::milliseconds(rpc_timeout_millis));

      StartGameResponsePB::Status status;
      if (!host_utils::StartGame(console_id, p1_stub_, &status)) {
        LOG(ERROR) << "Game start call failed.";
        continue;
      }

      if (status == StartGameResponsePB::SUCCESS) {
        LOG(INFO) << "Start game RPC number " << i + 1
                  << " succeeded and indicated a started game.";
        return true;
      }

      LOG(ERROR) << "Start game RPC number " << i + 1
                 << " succeeded but the server response status was "
                 << StartGameResponsePB::Status_Name(status);

      std::this_thread::sleep_for(
          std::chrono::milliseconds(wait_per_attempt_millis));
    }

    LOG(ERROR) << "Failed to start game";

    return false;
  }

  static const char kConsoleName[];
  static const char kRomName[];

  NetplayServerTestFixture fixture_;
  shared_ptr<NetPlayServerService::Stub> p1_stub_;
  shared_ptr<NetPlayServerService::Stub> p2_stub_;
};

const char NetplayIntegrationTest::kConsoleName[] =
    "integration-test-console-name";
const char NetplayIntegrationTest::kRomName[] = "integration-test-rom-name";

// -----------------------------------------------------------------------------
// A button coder that encodes a 32 bit integer in the x_axis field of the
// KeyStatePB proto.

class IntegerCoder : public ButtonCoderInterface<uint32_t> {
 public:
  bool EncodeButtons(const uint32_t& in,
                     KeyStatePB* buttons_out) const override {
    buttons_out->set_x_axis(in);
    return true;
  }
  bool DecodeButtons(const KeyStatePB& buttons_in,
                     uint32_t* out) const override {
    *out = buttons_in.x_axis();
    return true;
  }
};

// -----------------------------------------------------------------------------
// Tests

TEST_F(NetplayIntegrationTest, StartGameAndTransmitButtons) {
  // Player 1 creates a game.
  int console_id;
  {
    MakeConsoleResponsePB::Status make_console_status =
        MakeConsoleResponsePB::UNKNOWN;
    ASSERT_TRUE(host_utils::MakeConsole(kConsoleName, kRomName, p1_stub_,
                                        &make_console_status, &console_id));
    EXPECT_EQ(
        MakeConsoleResponsePB::Status_Name(make_console_status),
        MakeConsoleResponsePB::Status_Name(MakeConsoleResponsePB::SUCCESS));
    EXPECT_GT(console_id, 0);
  }

  // Player 1 and 2 create clients to the returned console.
  NetplayClient<uint32_t> client_1(
      p1_stub_, unique_ptr<ButtonCoderInterface<uint32_t>>(new IntegerCoder()),
      0,  // delay_frames
      console_id);
  NetplayClient<uint32_t> client_2(
      p1_stub_, unique_ptr<ButtonCoderInterface<uint32_t>>(new IntegerCoder()),
      0,  // delay_frames
      console_id);

  // Player 1 requests port 1, player 2 requests port 2.
  {
    PlugControllerResponsePB::Status plug_controller_status =
        PlugControllerResponsePB::UNKNOWN;

    ASSERT_TRUE(client_1.PlugControllers({PORT_1}, &plug_controller_status));
    ASSERT_EQ(PlugControllerResponsePB::Status_Name(
                  PlugControllerResponsePB::SUCCESS),
              PlugControllerResponsePB::Status_Name(plug_controller_status));

    plug_controller_status = PlugControllerResponsePB::UNKNOWN;
    ASSERT_TRUE(client_2.PlugControllers({PORT_2}, &plug_controller_status));
    ASSERT_EQ(PlugControllerResponsePB::Status_Name(
                  PlugControllerResponsePB::SUCCESS),
              PlugControllerResponsePB::Status_Name(plug_controller_status));
  }

  EXPECT_THAT(client_1.local_ports(), UnorderedElementsAre(PORT_1));
  EXPECT_THAT(client_2.local_ports(), UnorderedElementsAre(PORT_2));

  // Get the event streams, start threads.
  auto event_stream_1 = client_1.MakeEventStreamHandler();
  auto event_stream_2 = client_2.MakeEventStreamHandler();
  ASSERT_NE(nullptr, event_stream_1);
  ASSERT_NE(nullptr, event_stream_2);

  thread stream_1_thread([&, this] {
    auto* stream = event_stream_1.get();
    ASSERT_TRUE(ReadyForGame(100, 500, stream));
    ASSERT_THAT(stream->local_ports(), UnorderedElementsAre(PORT_1));
    ASSERT_THAT(stream->remote_ports(), UnorderedElementsAre(PORT_2));

    uint32_t data = -1;

    // Frame 0
    EXPECT_TRUE(SendFrame(PORT_1, 0, stream));
    data = GetFrame(PORT_1, 0, stream);
    EXPECT_EQ(MakeFrameData(PORT_1, 0), data);
    data = GetFrame(PORT_2, 0, stream);
    EXPECT_EQ(MakeFrameData(PORT_2, 0), data);

    // Frame 1
    EXPECT_TRUE(SendFrame(PORT_1, 1, stream));
    data = GetFrame(PORT_1, 1, stream);
    EXPECT_EQ(MakeFrameData(PORT_1, 1), data);
    data = GetFrame(PORT_2, 1, stream);
    EXPECT_EQ(MakeFrameData(PORT_2, 1), data);

    // Frame 2
    EXPECT_TRUE(SendFrame(PORT_1, 2, stream));
    data = GetFrame(PORT_1, 2, stream);
    EXPECT_EQ(MakeFrameData(PORT_1, 2), data);
    data = GetFrame(PORT_2, 2, stream);
    EXPECT_EQ(MakeFrameData(PORT_2, 2), data);
  });

  thread stream_2_thread([&, this] {
    auto* stream = event_stream_2.get();
    ASSERT_TRUE(ReadyForGame(100, 500, stream));
    ASSERT_THAT(stream->local_ports(), UnorderedElementsAre(PORT_2));
    ASSERT_THAT(stream->remote_ports(), UnorderedElementsAre(PORT_1));

    uint32_t data = -1;

    // Frame 0
    EXPECT_TRUE(SendFrame(PORT_2, 0, stream));
    data = GetFrame(PORT_1, 0, stream);
    EXPECT_EQ(MakeFrameData(PORT_1, 0), data);
    data = GetFrame(PORT_2, 0, stream);
    EXPECT_EQ(MakeFrameData(PORT_2, 0), data);

    // Frame 1
    EXPECT_TRUE(SendFrame(PORT_2, 1, stream));
    data = GetFrame(PORT_1, 1, stream);
    EXPECT_EQ(MakeFrameData(PORT_1, 1), data);
    data = GetFrame(PORT_2, 1, stream);
    EXPECT_EQ(MakeFrameData(PORT_2, 1), data);

    // Frame 2
    EXPECT_TRUE(SendFrame(PORT_2, 2, stream));
    data = GetFrame(PORT_1, 2, stream);
    EXPECT_EQ(MakeFrameData(PORT_1, 2), data);
    data = GetFrame(PORT_2, 2, stream);
    EXPECT_EQ(MakeFrameData(PORT_2, 2), data);
  });

  // Player 1 starts the game.
  EXPECT_TRUE(StartGame(console_id, 10, 500, 500));

  stream_1_thread.join();
  stream_2_thread.join();

  // Close the streams.
  event_stream_1->TryCancel();
  event_stream_2->TryCancel();
}
