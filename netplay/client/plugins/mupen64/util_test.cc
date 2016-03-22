#include "client/plugins/mupen64/util.cc"

#include "base/netplayServiceProto.pb.h"
#include "gtest/gtest.h"

namespace util {

TEST(UtilsTest, M64RequestedIntToPort) {
  EXPECT_EQ(PORT_ANY, M64RequestedIntToPort(0));
  EXPECT_EQ(PORT_1, M64RequestedIntToPort(1));
  EXPECT_EQ(PORT_2, M64RequestedIntToPort(2));
  EXPECT_EQ(PORT_3, M64RequestedIntToPort(3));
  EXPECT_EQ(PORT_4, M64RequestedIntToPort(4));
  EXPECT_EQ(UNKNOWN, M64RequestedIntToPort(-1));
}

TEST(UtilsTest, PortToM64RequestedInt) {
  EXPECT_EQ(0, PortToM64RequestedInt(PORT_ANY));
  EXPECT_EQ(1, PortToM64RequestedInt(PORT_1));
  EXPECT_EQ(2, PortToM64RequestedInt(PORT_2));
  EXPECT_EQ(3, PortToM64RequestedInt(PORT_3));
  EXPECT_EQ(4, PortToM64RequestedInt(PORT_4));
  EXPECT_EQ(-1, PortToM64RequestedInt(UNKNOWN));
}

TEST(UtilsTest, PortToM64Port) {
  EXPECT_EQ(0, PortToM64Port(PORT_1));
  EXPECT_EQ(1, PortToM64Port(PORT_2));
  EXPECT_EQ(2, PortToM64Port(PORT_3));
  EXPECT_EQ(3, PortToM64Port(PORT_4));
  EXPECT_EQ(-1, PortToM64Port(PORT_ANY));
  EXPECT_EQ(-1, PortToM64Port(UNKNOWN));
}

TEST(UtilsTest, M64PortToPort) {
  EXPECT_EQ(PORT_1, M64PortToPort(0));
  EXPECT_EQ(PORT_2, M64PortToPort(1));
  EXPECT_EQ(PORT_3, M64PortToPort(2));
  EXPECT_EQ(PORT_4, M64PortToPort(3));
  EXPECT_EQ(UNKNOWN, M64PortToPort(4));
  EXPECT_EQ(UNKNOWN, M64PortToPort(-1));
}

}  // namespace util
