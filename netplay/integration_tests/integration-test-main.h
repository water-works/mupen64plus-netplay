#include <string>
#include <vector>

namespace integration_tests {

// Get a pointer to the static list of jars for integration tests.
std::vector<std::string>* GetJars();

// If an executable calls this method and is not linked against the integration 
// test main, this will result in a linker error.
void MUST_USE_INTEGRATION_TEST_MAIN();

}  // namespace
