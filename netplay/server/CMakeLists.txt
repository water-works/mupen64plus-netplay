# GRPC output:  server/NetplayServiceProto/NetPlayServerServiceGrpc.java
# Proto output: server/NetplayServiceProto/NetplayServiceProto.java

SET(NETPLAY_SERVER_PROTO_SRC
  ${CMAKE_SOURCE_DIR}/base/netplay-protos/netplayServiceProto.proto)

SET (
  NETPLAY_JAVA_GRPC_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/netplayprotos/NetPlayServerServiceGrpc.java)

SET (
  NETPLAY_JAVA_PROTO_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/netplayprotos/NetplayServiceProto.java)

# ------------------------------------------------------------------------------
# Generated GRPC Java definitions


# TODO(alexgolec): Refactor this out into the FindGRPC module
ADD_CUSTOM_COMMAND(
  OUTPUT ${NETPLAY_JAVA_GRPC_SRCS}
  COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
  ARGS
    --plugin=protoc-gen-grpc-java=${GRPC_PROTOC_JAVA_PLUGIN}
    --grpc-java_out=${CMAKE_CURRENT_BINARY_DIR}/src
    --proto_path=${CMAKE_SOURCE_DIR}/base
    ${CMAKE_SOURCE_DIR}/base/netplay-protos/netplayServiceProto.proto
  DEPENDS
    ${NETPLAY_SERVER_PROTO_SRC}
    ${PROTOBUF_PROTOC_EXECUTABLE}
    GrpcJavaProtocPlugin
  COMMENT "Building GRPC Java service definitions for base/netplay-protos/netplayServiceProto.proto"
)
SET_SOURCE_FILES_PROPERTIES(${NETPLAY_JAVA_GRPC_SRCS} PROPERTIES GENERATED TRUE)

# ------------------------------------------------------------------------------
# Generated protobuf Java definitions

# TODO(alexgolec): Refactor this out into its own module
ADD_CUSTOM_COMMAND(
  OUTPUT ${NETPLAY_JAVA_PROTO_SRCS}
  COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
  ARGS
    --java_out=${CMAKE_CURRENT_BINARY_DIR}/src
    --proto_path=${CMAKE_SOURCE_DIR}/base
    ${CMAKE_SOURCE_DIR}/base/netplay-protos/netplayServiceProto.proto
  DEPENDS
    ${NETPLAY_SERVER_PROTO_SRC}
    ${PROTOBUF_PROTOC_EXECUTABLE}
  COMMENT "Building Java Protobuf definitions for base/netplay-protos/netplayServiceProto.proto"
)
SET_SOURCE_FILES_PROPERTIES(${NETPLAY_JAVA_PROTO_SRCS} PROPERTIES GENERATED TRUE)

# ------------------------------------------------------------------------------
# Server JAR packaging

SET (TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
SET (SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
SET (JAR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/jars)

# Names of various JAR files
SET (GRPC_JAR ${JAR_DIR}/grpc-all-0.9.0.jar)
SET (PROTO_JAR ${JAR_DIR}/protobuf-java-3.0.0-beta-1.jar)
SET (GUAVA_JAR ${JAR_DIR}/guava-18.0.jar)
SET (COMMONS_CLI_JAR ${JAR_DIR}/commons-cli-1.3.1.jar)
SET (COMMONS_LOGGING_JAR ${JAR_DIR}/commons-logging-1.2.jar)
SET (JSR_JAR ${JAR_DIR}/jsr311-api-1.1.1.jar)
SET (JUNIT_JAR ${JAR_DIR}/junit-4.12.jar)
SET (NETTY_JAR ${JAR_DIR}/netty-all-4.1.0.Beta6.jar)
SET (GOOGLE_HTTP_CLIENT_JAR ${JAR_DIR}/google-http-client-1.20.0.jar)
SET (HAMCREST_JAR ${JAR_DIR}/hamcrest-core-1.3.jar)
SET (HPACK_JAR ${JAR_DIR}/hpack-v1.0.1.jar)

ADD_JAR (
  NetplayGRPC
  SOURCE
    ${NETPLAY_JAVA_PROTO_SRCS}
    ${NETPLAY_JAVA_GRPC_SRCS}
  INCLUDE_JARS
    ${GRPC_JAR}
    ${PROTO_JAR}
    ${GUAVA_JAR})
SET (NETPLAY_PROTO_JAR ${CMAKE_CURRENT_SOURCE_DIR}/NetplayGRPC.jar)

ADD_JAR (
  NetplayServer
  SOURCES
    ${SRC_DIR}/netplayServer/Client.java
    ${SRC_DIR}/netplayServer/Console.java
    ${SRC_DIR}/netplayServer/Player.java
    ${SRC_DIR}/netplayServer/PlugRequestException.java
    ${SRC_DIR}/netplayServer/Server.java
    ${SRC_DIR}/netplayServer/ServerStart.java
  INCLUDE_JARS
    ${NETPLAY_PROTO_JAR}
    ${COMMONS_CLI_JAR}
    ${COMMONS_LOGGING_JAR}
    ${GRPC_JAR}
    ${GUAVA_JAR}
    ${JSR_JAR}
    ${PROTO_JAR}
    ${GOOGLE_HTTP_CLIENT_JAR})
SET (NETPLAY_SERVER_JAR ${CMAKE_CURRENT_SOURCE_DIR}/NetplayServer.jar)

# ------------------------------------------------------------------------------
# Tests

FUNCTION (ADD_NETPLAY_JAVA_TEST TEST_JAR TEST_CLASSNAME)
  IF (NOT TEST_CLASSNAME)
    MESSAGE (SEND_ERROR
      "ERROR: ADD_NETPLAY_JAVA_TEST called with a test classname")
    RETURN()
  ENDIF()
  IF (NOT TEST_JAR)
    MESSAGE (SEND_ERROR
      "ERROR: ADD_NETPLAY_JAVA_TEST called with a test jar")
    RETURN()
  ENDIF()

  SET (CP_SEP ":")
  STRING (CONCAT CLASSPATH
              ${TEST_JAR}
    ${CP_SEP} ${HAMCREST_JAR}
    ${CP_SEP} ${JUNIT_JAR}
    ${CP_SEP} ${NETPLAY_SERVER_JAR}
    ${CP_SEP} ${NETPLAY_PROTO_JAR}
    ${CP_SEP} ${PROTO_JAR}
    ${CP_SEP} ${GUAVA_JAR}
    ${CP_SEP} ${COMMONS_LOGGING_JAR}
    ${CP_SEP} ${GRPC_JAR}
  )

  ADD_TEST (${TEST_CLASSNAME}_test
    ${Java_JAVA_EXECUTABLE} -cp ${CLASSPATH}
    org.junit.runner.JUnitCore ${TEST_CLASSNAME})
ENDFUNCTION ()

ADD_JAR (
  ConsoleTest ${TEST_DIR}/netplayServer/ConsoleTest.java
  INCLUDE_JARS
    ${NETPLAY_SERVER_JAR}
    ${NETPLAY_PROTO_JAR}
    ${GRPC_JAR}
    ${JUNIT_JAR}
    ${GUAVA_JAR}
    ${PROTO_JAR})
ADD_NETPLAY_JAVA_TEST (ConsoleTest.jar netplayServer.ConsoleTest)

ADD_JAR (
  ClientTest ${TEST_DIR}/netplayServer/ClientTest.java
  INCLUDE_JARS
    ${NETPLAY_SERVER_JAR}
    ${NETPLAY_PROTO_JAR}
    ${GRPC_JAR}
    ${JUNIT_JAR}
    ${GUAVA_JAR}
    ${PROTO_JAR})
ADD_NETPLAY_JAVA_TEST (ClientTest.jar netplayServer.ClientTest)

ADD_JAR (
  ServerTest ${TEST_DIR}/netplayServer/ServerTest.java
  INCLUDE_JARS
    ${NETPLAY_SERVER_JAR}
    ${NETPLAY_PROTO_JAR}
    ${GRPC_JAR}
    ${JUNIT_JAR}
    ${GUAVA_JAR}
    ${PROTO_JAR})
ADD_NETPLAY_JAVA_TEST (ServerTest.jar netplayServer.ServerTest)

# List of jars for use in outside tests, such as integration tests.
SET (
  NETPLAY_JARS
  ${GRPC_JAR}
  ${PROTO_JAR}
  ${GUAVA_JAR}
  ${COMMONS_CLI_JAR}
  ${COMMONS_LOGGING_JAR}
  ${JSR_JAR}
  ${NETPLAY_PROTO_JAR}
  ${NETPLAY_SERVER_JAR}
  ${NETTY_JAR}
  ${HPACK_JAR}
  PARENT_SCOPE)
