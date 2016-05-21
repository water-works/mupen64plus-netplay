# ------------------------------------------------------------------------------
# Libs

ADD_LIBRARY (NetplayServer netplay-server.cc)
ADD_LIBRARY (Console console.cc)

SET (NETPLAY_SERVER_LIBS
  NetplayServer
  Console)

# ------------------------------------------------------------------------------
# Tests

SET (NETPLAY_SERVER_TEST_LIBS
  ${NETPLAY_TEST_LIBS}
  ${NETPLAY_SERVER_LIBS})

ADD_EXECUTABLE (NetplayServer_test netplay-server_test.cc)
TARGET_LINK_LIBRARIES (NetplayServer_test ${NETPLAY_SERVER_TEST_LIBS})
GTEST_ADD_TESTS (NetplayServer_test ${GTEST_ARGS} netplay-server_test.cc)
