CMAKE_MINIMUM_REQUIRED (VERSION 3.2)

include (ExternalProject)

# ------------------------------------------------------------------------------
# Global variables.

# Installation directory of all required projects.
SET (INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/usr)

# ------------------------------------------------------------------------------
# GRPC C/C++

SET (GRPC_PREFIX ${CMAKE_SOURCE_DIR}/third-party/grpc)
SET (GRPC_MAKE_ARGS prefix=${INSTALL_PREFIX} REQUIRE_CUSTOM_LIBRARIES_opt=true)

ExternalProject_Add (
  GRPC
  PREFIX ${GRPC_PREFIX}

  # Configure step (skip)
  SOURCE_DIR ${GRPC_PREFIX}
  CONFIGURE_COMMAND ""

  # Build step
  BINARY_DIR ${GRPC_PREFIX}
  BUILD_COMMAND make ${GRPC_MAKE_ARGS}

  # Install step
  INSTALL_DIR ${INSTALL_PREFIX}
  INSTALL_COMMAND make install ${GRPC_MAKE_ARGS}
)

# ------------------------------------------------------------------------------
# Proto3
#
# Note we use the proto3 repository cloned by the grpc submodule.

SET (PROTO3_PREFIX ${CMAKE_SOURCE_DIR}/third-party/grpc/third_party/protobuf)

ExternalProject_Add (
  Protobuf3
  PREFIX ${PROTO3_PREFIX}
  DEPENDS GRPC

  # Configure step (skip)
  SOURCE_DIR ${PROTO3_PREFIX}
  CONFIGURE_COMMAND ""

  # Build step
  BINARY_DIR ${PROTO3_PREFIX}
  BUILD_COMMAND ""

  # Install step
  INSTALL_DIR ${INSTALL_PREFIX}
  INSTALL_COMMAND make install prefix=${INSTALL_PREFIX}
)

# ------------------------------------------------------------------------------
# GTest

SET (GTEST_PREFIX ${CMAKE_SOURCE_DIR}/third-party/googletest/googletest)

ExternalProject_Add (
  GTest
  PREFIX ${GTEST_PREFIX}

  # Configure step (cmake by default)
  SOURCE_DIR ${GTEST_PREFIX}

  # Build step
  BINARY_DIR ${GTEST_PREFIX}
  BUILD_COMMAND make

  # Install step (skip)
  INSTALL_COMMAND ""
)

# ------------------------------------------------------------------------------
# GMock

SET (GMOCK_PREFIX ${CMAKE_SOURCE_DIR}/third-party/googletest/googlemock)

ExternalProject_Add (
  GMock
  PREFIX ${GMOCK_PREFIX}

  # Configure step (cmake by default)
  SOURCE_DIR ${GMOCK_PREFIX}

  # Build step
  BINARY_DIR ${GMOCK_PREFIX}
  BUILD_COMMAND make

  # Install step (skip)
  INSTALL_COMMAND ""
)

# ------------------------------------------------------------------------------
# mupen64plus-core

SET (MUPEN64PLUS_CORE_PREFIX ${CMAKE_SOURCE_DIR}/mupen64plus-core/projects/unix)

ExternalProject_Add(
  Mupen64Plus_Core
  PREFIX ${MUPEN64PLUS_CORE_PREFIX}

  # Configure step (skip)
  SOURCE_DIR ${MUPEN64PLUS_CORE_PREFIX}
  CONFIGURE_COMMAND ""

  # Build step
  BINARY_DIR ${MUPEN64PLUS_CORE_PREFIX}
  BUILD_COMMAND make all DEBUG=1

  # Install step (skip)
  INSTALL_COMMAND ""
)

ExternalProject_Add_Step (
  Mupen64Plus_Core forceconfigure
  COMMAND ${CMAKE_COMMAND} -E echo "Force configure of mupen64plus-core"
  DEPENDEES update
  DEPENDERS configure
  ALWAYS 1)

# ------------------------------------------------------------------------------
# mupen64plus-ui-console

SET (MUPEN64PLUS_UI_CONSOLE_PREFIX
  ${CMAKE_SOURCE_DIR}/mupen64plus-ui-console/projects/unix)

ExternalProject_Add(
  Mupen64Plus_Ui_Console
  PREFIX ${MUPEN64PLUS_UI_CONSOLE_PREFIX}

  # Configure step (skip)
  SOURCE_DIR ${MUPEN64PLUS_UI_CONSOLE_PREFIX}
  CONFIGURE_COMMAND ""

  # Build step
  BINARY_DIR ${MUPEN64PLUS_UI_CONSOLE_PREFIX}
  BUILD_COMMAND make all

  # Install step (skip)
  INSTALL_COMMAND ""
)

ExternalProject_Add_Step (
  Mupen64Plus_Ui_Console forceconfigure
  COMMAND ${CMAKE_COMMAND} -E echo "Force configure of mupen64plus-ui-console"
  DEPENDEES update
  DEPENDERS configure
  ALWAYS 1)

# ------------------------------------------------------------------------------
# GFlags

SET (GFLAGS_PREFIX ${CMAKE_SOURCE_DIR}/third-party/gflags)

ExternalProject_Add(
  GFlags
  PREFIX ${GFLAGS_PREFIX}

  SOURCE_DIR ${GFLAGS_PREFIX}
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX:PATH=${INSTALL_PREFIX}
    -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=True
)

# ------------------------------------------------------------------------------
# GLog

SET (GLOG_PREFIX ${CMAKE_SOURCE_DIR}/third-party/glog)

ExternalProject_Add(
  GLog
  PREFIX ${GLOG_PREFIX}

  SOURCE_DIR ${GLOG_PREFIX}
  CONFIGURE_COMMAND ${GLOG_PREFIX}/configure --prefix=${INSTALL_PREFIX}
)

# ------------------------------------------------------------------------------
# Netplay

SET (NETPLAY_PREFIX ${CMAKE_SOURCE_DIR}/netplay)

ExternalProject_Add (
  Netplay
  PREFIX ${NETPLAY_PREFIX}
  DEPENDS
    Protobuf3
    GRPC
    GTest
    GMock
    GLog
    Mupen64Plus_Core
    Mupen64Plus_Ui_Console

  # Configure step
  SOURCE_DIR ${NETPLAY_PREFIX}
  CMAKE_ARGS
    -DGRPC_ROOT:string=${INSTALL_PREFIX}
    -DGTEST_ROOT:string=${GTEST_PREFIX}
    -DGMOCK_ROOT:string=${GMOCK_PREFIX}
    -DMUPEN64_ROOT:string=${CMAKE_SOURCE_DIR}/mupen64plus-core/src/api
    -DCMAKE_LIBRARY_PATH:string=${INSTALL_PREFIX}/lib
    -DCMAKE_PROGRAM_PATH:string=${INSTALL_PREFIX}/bin
    -DCMAKE_INCLUDE_PATH:string=${INSTALL_PREFIX}/include
    # TODO(alexgolec): LLVM 7.0.0 on OSX complains about missing overrides in 
    # GRPC headers. Hopefully those headers will eventually be updated, so 
    # remove this once that happens.
    -DCMAKE_CXX_FLAGS:string=-Wno-inconsistent-missing-override
    -DCMAKE_BUILD_TYPE:string=Debug

  # Build step
  BINARY_DIR ${NETPLAY_PREFIX}
  BUILD_COMMAND make

  # Install step (skip)
  INSTALL_COMMAND ""
)

ExternalProject_Add_Step (
  Netplay forceconfigure
  COMMAND ${CMAKE_COMMAND} -E echo "Force configure of netplay"
  DEPENDEES update
  DEPENDERS configure
  ALWAYS 1)
