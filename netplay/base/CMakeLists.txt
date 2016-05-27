# ------------------------------------------------------------------------------
# C++ outputs

INCLUDE_DIRECTORIES (${CMAKE_CURRENT_SOURCE_DIR})

GRPC_GENERATE_CPP (
  NETPLAY_SERVICE_GRPC_CPP_SRCS
  NETPLAY_SERVICE_GRPC_CPP_HDRS
  netplay-protos/netplayServiceProto.proto)

PROTOBUF_GENERATE_CPP (
  NETPLAY_SERVICE_PROTOS_SRCS
  NETPLAY_SERVICE_PROTOS_HDRS
  netplay-protos/netplayServiceProto.proto)

PROTOBUF_GENERATE_CPP (
  TIMINGS_PROTOS_SRCS
  TIMINGS_PROTOS_HDRS
  netplay-protos/timings.proto)

ADD_LIBRARY (NetplayServiceProtos ${NETPLAY_SERVICE_PROTOS_SRCS})
ADD_LIBRARY (
  NetplayServiceGRPCCpp
  ${NETPLAY_SERVICE_GRPC_CPP_SRCS} 
  ${NETPLAY_SERVICE_PROTOS_SRCS})
ADD_LIBRARY (TimingsProtos ${TIMINGS_PROTOS_SRCS})
