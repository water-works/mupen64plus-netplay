# ------------------------------------------------------------------------------
# Libraries

ADD_LIBRARY (NetplayServerTestFixture netplay-server-test-fixture.cc)
ADD_LIBRARY (IntegrationTestMain integration-test-main.cc)

# Note the main lib is not in this array
SET (INTEGRATION_TEST_LIBS
  NetplayServerTestFixture
  ${JNI_LIBRARIES}
  ${NETPLAY_TEST_LIBS})

# ------------------------------------------------------------------------------
# Executables and tests

# Here is a sample test:
ADD_EXECUTABLE (IntegrationTestStaffolding_test integration-test-main.cc)
TARGET_LINK_LIBRARIES (IntegrationTestStaffolding_test ${INTEGRATION_TEST_LIBS})
ADD_TEST (
  NAME IntegrationTestStaffolding_test
  COMMAND IntegrationTestStaffolding_test ${NETPLAY_JARS})

ADD_EXECUTABLE (IntegrationTests_test integration-tests.cc)
TARGET_LINK_LIBRARIES (
  IntegrationTests_test
  IntegrationTestMain
  ${INTEGRATION_TEST_LIBS})
ADD_TEST (
  NAME IntegrationTests_test
  COMMAND IntegrationTests_test ${NETPLAY_JARS})
