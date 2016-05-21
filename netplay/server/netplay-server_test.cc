#include "server/netplay-server.h"

#include "gtest/gtest.h"

namespace server {

class NetplayServerTest : public ::testing::Test {
 public:
  NetplayServerTest() : server_(true) {}

  NetplayServer server_;
};

TEST_F(NetplayServerTest, PingReturnsOK) {
}

}  // namespace server
