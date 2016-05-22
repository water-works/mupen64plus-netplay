#include "server/netplay-server.h"

#include "gtest/gtest.h"

namespace server {

class NetplayServerTest : public ::testing::Test {
 public:
  NetplayServerTest() : server_(true) {}

  NetplayServer server_;
  grpc::ServerContext dummy_context_;
};

TEST_F(NetplayServerTest, PingReturnsOK) {
  PingPB request, response;
  EXPECT_TRUE(server_.Ping(&dummy_context_, &request, &response).ok());
}

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
  EXPECT_EQ(101,  response.console_id());
}

}  // namespace server
