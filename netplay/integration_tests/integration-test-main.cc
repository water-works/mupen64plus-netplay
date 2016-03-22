#include <iostream>
#include <string>
#include <vector>

#include "glog/logging.h"
#include "gtest/gtest.h"
#include "integration_tests/netplay-server-test-fixture.h"

using std::vector;
using std::string;

namespace integration_tests {

vector<string>* GetJars() {
  static vector<string> jars;
  return &jars;
}

void MUST_USE_INTEGRATION_TEST_MAIN() {}

}  // namespace integration_tests

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::InstallFailureSignalHandler();

  vector<string>* jars = integration_tests::GetJars();
  for (int i = 1; i < argc; ++i) {
    jars->push_back(argv[i]);
  }

  return RUN_ALL_TESTS();
}
