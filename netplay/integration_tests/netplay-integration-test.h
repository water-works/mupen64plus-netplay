#ifndef INTEGRATION_TEST_NETPLAY_INTEGRATION_TEST_H_
#define INTEGRATION_TEST_NETPLAY_INTEGRATION_TEST_H_

#include <vector>
#include <string>

#include "integration_tests/netplay-server-test-fixture.h"
#include "gtest/gtest.h"

using std::vector;
using std::string;

// The global vector of string jar arguments. This must be populated *before* 
// any code is run that would initialize NetplayIntegrationTest, in particular 
// the gtest main method.
extern vector<string> GlobalJars;

class NetplayIntegrationTest : public ::testing::Test {
 protected:
  NetplayServerTestFixture server_;
};

#endif  // INTEGRATION_TEST_NETPLAY_INTEGRATION_TEST_H_
