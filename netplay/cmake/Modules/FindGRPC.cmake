# Locate the GRPC package
#
# Accepts the following variables as input:
#  GRPC_ROOT
#
# Defines the following variables:
#
#  GRPC_FOUND - Found the GRPC headers and library
#  GRPC_INCLUDE_DIRS - Include directories containing grpc and grpc++
#  GRPC_LIBRARY - libgrpc
#  GRPCPP_LIBRARY - libgrpc++
#  GRPC_PROTOC_CPP_PLUGIN - GRPC protoc C++ compiler plugin

# ------------------------------------------------------------------------------
# c++ target creation function

FUNCTION (GRPC_GENERATE_CPP SRCS HDRS)
  IF (NOT ARGN)
    MESSAGE (SEND_ERROR "Error: GRPC_GENERATE_CPP() called without any proto files")
    RETURN()
  ENDIF()

  SET (${SRCS})
  SET (${HDRS})
  FOREACH (FIL ${ARGN})
    GET_FILENAME_COMPONENT (FIL_DIR ${FIL} DIRECTORY)
    GET_FILENAME_COMPONENT (ABS_FIL ${FIL} ABSOLUTE)
    GET_FILENAME_COMPONENT (FIL_WE ${FIL} NAME_WE)
    GET_FILENAME_COMPONENT (FIL_ABS_DIR ${ABS_FIL} DIRECTORY)

    SET (_CC_SRC "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.grpc.pb.cc")
    SET (_CC_HDR "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.grpc.pb.h")

    LIST (APPEND ${SRCS} ${_CC_SRC})
    LIST (APPEND ${HDRS} ${_CC_HDR})

    ADD_CUSTOM_COMMAND(
      OUTPUT ${_CC_SRC} ${_CC_HDR}
      COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
      ARGS -I ${FIL_ABS_DIR}
           --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
           --plugin=protoc-gen-grpc=${GRPC_PROTOC_CPP_PLUGIN} 
           ${ABS_FIL}
      DEPENDS ${ABS_FIL} ${PROTOBUF_PROTOC_EXECUTABLE}
      COMMENT "Building GRPC C++ service definitions for ${FIL}"
      VERBATIM)
  ENDFOREACH()

  SET_SOURCE_FILES_PROPERTIES(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
  SET (${SRCS} ${${SRCS}} PARENT_SCOPE)
  SET (${HDRS} ${${HDRS}} PARENT_SCOPE)
ENDFUNCTION()

# ------------------------------------------------------------------------------
# Include dirs

FIND_PATH (
  GRPC_INCLUDE_DIR
  NAMES
    grpc/grpc.h
    grpc++/channel_arguments.h
    grpc++/channel_interface.h
    grpc++/client_context.h
    grpc++/config.h
    grpc++/credentials.h
  HINTS
    $ENV{GRPC_ROOT}/include
    ${GRPC_ROOT}/include
  DOC "GRPC install root"
)
MARK_AS_ADVANCED (GRPC_INSTALL_ROOT)

# ------------------------------------------------------------------------------
# Libraries

FIND_LIBRARY (
  GRPC_LIBRARY
  NAMES
    grpc
  HINTS
  ${GRPC_ROOT}/lib
  DOC "GRPC C library"
)
MARK_AS_ADVANCED (GRPC_LIBRARY)

FIND_LIBRARY (
  GRPC_UNSECURE_LIBRARY
  NAMES
    grpc_unsecure
  HINTS
    ${GRPC_ROOT}/lib
  DOC "GRPC insecure library"
)
MARK_AS_ADVANCED (GRPC_UNSECURE_LIBRARY)

FIND_LIBRARY (
  GRPCPP_LIBRARY
  NAMES
    grpc++
  HINTS
    ${GRPC_ROOT}/lib
  DOC "GRPC C++ library"
)
MARK_AS_ADVANCED (GRPCPP_LIBRARY)

# ------------------------------------------------------------------------------
# GRPC protoc plugins

FIND_PROGRAM(
  GRPC_PROTOC_CPP_PLUGIN
  grpc_cpp_plugin
  HINTS
    $ENV{GRPC_ROOT}/bin
    ${GRPC_ROOT}/bin
)
MARK_AS_ADVANCED (GRPC_PROTOC_CPP_PLUGIN)

# ------------------------------------------------------------------------------
# Wrapup and variable setting

INCLUDE (FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
  GRPC
  DEFAULT_MSG
  GRPC_LIBRARY
  GRPC_UNSECURE_LIBRARY
  GRPCPP_LIBRARY
  GRPC_INCLUDE_DIR
  GRPC_PROTOC_CPP_PLUGIN
)

if (GRPC_FOUND)
  SET (GRPC_INCLUDE_DIRS ${GRPC_INSTALL_ROOT}/include)
  SET (GRPC_LIBRARY ${GRPC_LIBRARY_PATH})
ENDIF()
