// Note this test also contains tests for host-utils.h

#include "client/client.h"

#include <algorithm>
#include <memory>
#include <sstream>
#include <thread>
#include <vector>

#include "client/host-utils.h"
#include "client/mocks.h"
#include "client/test-utils.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using std::string;
using std::stringstream;
using std::vector;

using ::testing::_;
using ::testing::AtMost;
using ::testing::DoAll;
using ::testing::Ref;
using ::testing::Return;
using ::testing::SetArgPointee;

class NetplayClientTest : public ::testing::Test {
 protected:
  typedef NetplayClient<string> StringClient;

  static const int kConsoleId;
  static const char kRomMD5[];
  static const int kClientId;
  static const int kDelayFrames;
  static const char kConsoleTitle[];
  static const char kRomName[];

  NetplayClientTest()
      : mock_stub_(new MockNetPlayServerServiceStub()),
        mock_coder_(new MockButtonCoder<string>()),
        client_(new StringClient(mock_stub_, std::move(mock_coder_),
                                 kDelayFrames)) {}

  std::shared_ptr<MockNetPlayServerServiceStub> mock_stub_;
  std::unique_ptr<MockButtonCoder<string>> mock_coder_;
  std::unique_ptr<StringClient> client_;
};

const int NetplayClientTest::kConsoleId = 101;
const char NetplayClientTest::kRomMD5[] = "abcdefg1234567";
const int NetplayClientTest::kClientId = 1001;
const int NetplayClientTest::kDelayFrames = 5;
const char NetplayClientTest::kConsoleTitle[] = "console title";
const char NetplayClientTest::kRomName[] = "rom name";

TEST_F(NetplayClientTest, Accessors) {
  EXPECT_EQ(kDelayFrames, client_->delay_frames());
  EXPECT_EQ(mock_stub_, client_->stub());

  // These are set by PlugControllers.
  EXPECT_EQ(-1, client_->console_id());
  EXPECT_EQ(-1, client_->client_id());
}

// -----------------------------------------------------------------------------
// MakeConsole

TEST_F(NetplayClientTest, MakeConsoleSuccess) {
  const int kNewConsoleId = kConsoleId + 1;

  // Response
  MakeConsoleResponsePB response;
  response.set_status(MakeConsoleResponsePB::SUCCESS);
  response.set_console_id(kNewConsoleId);

  // Set mock expectations
  EXPECT_CALL(*mock_stub_, MakeConsole(_, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(response), Return(grpc::Status::OK)));

  MakeConsoleResponsePB::Status status;
  int console_id = -1;
  ASSERT_TRUE(host_utils::MakeConsole(kConsoleTitle, kRomName, mock_stub_,
                                      &status, &console_id));
  EXPECT_EQ(kNewConsoleId, console_id);
}

TEST_F(NetplayClientTest, MakeConsoleRpcFailure) {
  // Set mock expectations
  EXPECT_CALL(*mock_stub_, MakeConsole(_, _, _))
      .WillOnce(Return(grpc::Status::CANCELLED));

  MakeConsoleResponsePB::Status status;
  int console_id = -1;
  EXPECT_FALSE(host_utils::MakeConsole(kConsoleTitle, kRomName,
                                       mock_stub_, &status, &console_id));
}

TEST_F(NetplayClientTest, MakeConsoleRequestFailure) {
  // Response
  MakeConsoleResponsePB response;
  response.set_status(MakeConsoleResponsePB::UNSPECIFIED_FAILURE);

  // Set mock expectations
  EXPECT_CALL(*mock_stub_, MakeConsole(_, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(response), Return(grpc::Status::OK)));

  MakeConsoleResponsePB::Status status;
  int console_id = -1;
  ASSERT_TRUE(host_utils::MakeConsole(kConsoleTitle, kRomName, mock_stub_,
                                      &status, &console_id));
  EXPECT_EQ(MakeConsoleResponsePB::UNSPECIFIED_FAILURE, status);
}

// -----------------------------------------------------------------------------
// StartGame

TEST_F(NetplayClientTest, StartGameSuccess) {
  StartGameResponsePB response;
  response.set_status(StartGameResponsePB::SUCCESS);
  EXPECT_CALL(*mock_stub_, StartGame(_, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(response), Return(grpc::Status::OK)));

  StartGameResponsePB::Status status = StartGameResponsePB::UNSPECIFIED_FAILURE;
  ASSERT_TRUE(host_utils::StartGame(kConsoleId, mock_stub_, &status));
  EXPECT_EQ(StartGameResponsePB::SUCCESS, status);
}

TEST_F(NetplayClientTest, StartGameRPCFailure) {
  StartGameResponsePB response;
  response.set_status(StartGameResponsePB::SUCCESS);
  EXPECT_CALL(*mock_stub_, StartGame(_, _, _))
      .WillOnce(
          DoAll(SetArgPointee<2>(response), Return(grpc::Status::CANCELLED)));

  StartGameResponsePB::Status status = StartGameResponsePB::UNSPECIFIED_FAILURE;
  EXPECT_FALSE(host_utils::StartGame(kConsoleId, mock_stub_, &status));
}

// -----------------------------------------------------------------------------
// PlugControllers

TEST_F(NetplayClientTest, PlugControllerSuccess) {
  const vector<Port> requested_ports = {PORT_1, PORT_2, PORT_ANY, PORT_ANY};
  for (int i = 0; i < requested_ports.size(); ++i) {
    const vector<Port> ports(requested_ports.begin(),
                             requested_ports.begin() + 1);

    PlugControllerResponsePB response;
    response.set_console_id(kConsoleId);
    response.set_status(PlugControllerResponsePB::SUCCESS);
    response.set_client_id(kClientId);
    EXPECT_CALL(*mock_stub_, PlugController(_, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(response), Return(grpc::Status::OK)));

    PlugControllerResponsePB::Status status =
        PlugControllerResponsePB::UNSPECIFIED_FAILURE;
    ASSERT_TRUE(client_->PlugControllers(kConsoleId, kRomMD5, ports, &status));
    ASSERT_EQ(status, PlugControllerResponsePB::SUCCESS);
    EXPECT_EQ(kClientId, client_->client_id());
  }

  const auto& timings = *client_->mutable_timings();
  EXPECT_EQ(8, timings.event_size());
  EXPECT_GT(timings.event(0).plug_controller_request(), 0);
  EXPECT_GT(timings.event(1).plug_controller_response(), 0);
  EXPECT_GT(timings.event(2).plug_controller_request(), 0);
  EXPECT_GT(timings.event(3).plug_controller_response(), 0);
  EXPECT_GT(timings.event(4).plug_controller_request(), 0);
  EXPECT_GT(timings.event(5).plug_controller_response(), 0);
  EXPECT_GT(timings.event(6).plug_controller_request(), 0);
  EXPECT_GT(timings.event(7).plug_controller_response(), 0);
}

TEST_F(NetplayClientTest, PlugControllerEmptyPorts) {
  PlugControllerResponsePB response;
  response.set_console_id(kConsoleId);
  response.set_status(PlugControllerResponsePB::SUCCESS);
  EXPECT_CALL(*mock_stub_, PlugController(_, _, _))
      .Times(AtMost(1))
      .WillOnce(DoAll(SetArgPointee<2>(response), Return(grpc::Status::OK)));

  PlugControllerResponsePB::Status status =
      PlugControllerResponsePB::UNSPECIFIED_FAILURE;
  EXPECT_FALSE(client_->PlugControllers(kConsoleId, kRomMD5, {}, &status));
}

TEST_F(NetplayClientTest, PlugControllerMismatchedConsoleId) {
  PlugControllerResponsePB response;
  response.set_console_id(kConsoleId + 1);
  response.set_status(PlugControllerResponsePB::SUCCESS);
  EXPECT_CALL(*mock_stub_, PlugController(_, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(response), Return(grpc::Status::OK)));

  PlugControllerResponsePB::Status status =
      PlugControllerResponsePB::UNSPECIFIED_FAILURE;
  EXPECT_FALSE(
      client_->PlugControllers(kConsoleId, kRomMD5, {PORT_1}, &status));
}

TEST_F(NetplayClientTest, PlugControllerRPCFailure) {
  EXPECT_CALL(*mock_stub_, PlugController(_, _, _))
      .WillOnce(Return(grpc::Status::CANCELLED));

  PlugControllerResponsePB::Status status =
      PlugControllerResponsePB::UNSPECIFIED_FAILURE;
  EXPECT_FALSE(
      client_->PlugControllers(kConsoleId, kRomMD5, {PORT_1}, &status));
}
