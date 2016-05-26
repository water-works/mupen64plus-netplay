#include "server/netplay-server.h"

#include "gtest/gtest.h"

namespace server {

class NetplayServerTest : public ::testing::Test {
 protected:
  NetplayServerTest() : server_(true) {}

  NetplayServer server_;
  grpc::ServerContext dummy_context_;
};

// -----------------------------------------------------------------------------
// Ping

TEST_F(NetplayServerTest, PingReturnsOK) {
  PingPB request, response;
  EXPECT_TRUE(server_.Ping(&dummy_context_, &request, &response).ok());
}

// -----------------------------------------------------------------------------
// MakeConsole

TEST_F(NetplayServerTest, MakeConsoleSuccess) {
  // Set the generator to a specific value.
  server_.console_id_generator_ = 100;

  MakeConsoleRequestPB request;
  request.set_console_title("console title");
  request.set_rom_name("rom name");
  request.set_rom_file_md5("rom md5");
  MakeConsoleResponsePB response;
  ASSERT_TRUE(server_.MakeConsole(&dummy_context_, &request, &response).ok());

  EXPECT_EQ(MakeConsoleResponsePB::SUCCESS, response.status());
  EXPECT_EQ(101, response.console_id());
}

// -----------------------------------------------------------------------------
// PlugController

TEST_F(NetplayServerTest, PlugControllerSuccess) {
  // Create a console.
  MakeConsoleRequestPB make_console_request;
  make_console_request.set_console_title("console title");
  make_console_request.set_rom_name("rom name");
  make_console_request.set_rom_file_md5("rom md5");
  MakeConsoleResponsePB make_console_response;
  ASSERT_TRUE(server_.MakeConsole(&dummy_context_, &make_console_request,
                                  &make_console_response)
                  .ok());

  // Request a port allocation.
  PlugControllerRequestPB request;
  request.set_console_id(make_console_response.console_id());
  request.set_delay_frames(4);
  request.set_requested_port_1(Port::PORT_ANY);
  PlugControllerResponsePB response;
  ASSERT_TRUE(
      server_.PlugController(&dummy_context_, &request, &response).ok());
}

TEST_F(NetplayServerTest, PlugControllerNoSuchConsole) {
  // Request a port allocation.
  PlugControllerRequestPB request;
  request.set_console_id(999);
  request.set_delay_frames(4);
  request.set_requested_port_1(Port::PORT_ANY);
  PlugControllerResponsePB response;
  ASSERT_TRUE(
      server_.PlugController(&dummy_context_, &request, &response).ok());
  EXPECT_EQ(PlugControllerResponsePB::Status_Name(
                PlugControllerResponsePB::NO_SUCH_CONSOLE),
            PlugControllerResponsePB::Status_Name(response.status()));
}

// -----------------------------------------------------------------------------
// StartGame

TEST_F(NetplayServerTest, StartGameNotAllClientsHaveStreamsRegistered) {

}

}  // namespace server
