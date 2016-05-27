CMAKE_MINIMUM_REQUIRED (VERSION 3.2)
PROJECT (netplay)
ENABLE_TESTING ()

SET (CTEST_OUTPUT_ON_FAILURE ON)

SET (CMAKE_CXX_STANDARD 11)
SET (CMAKE_POSITION_INDEPENDENT_CODE ON)

SET (THREADS_PREFER_PTHREAD_FLAG ON)

SET(
  CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH} 
  ${CMAKE_SOURCE_DIR}/cmake/Modules/)

SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin)

# ------------------------------------------------------------------------------
# Find GMock and GTest

FIND_PACKAGE (GTest REQUIRED)
FIND_PACKAGE (Glog REQUIRED)
FIND_PACKAGE (GMock REQUIRED)
FIND_PACKAGE (Mupen64 REQUIRED)
FIND_PACKAGE (Protobuf REQUIRED)
FIND_PACKAGE (GRPC REQUIRED)
FIND_PACKAGE (Threads REQUIRED)

INCLUDE_DIRECTORIES (${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES (${GLOG_INCLUDE_DIRS})
INCLUDE_DIRECTORIES (${GTEST_INCLUDE_DIRS})
INCLUDE_DIRECTORIES (${GMOCK_INCLUDE_DIRS})
INCLUDE_DIRECTORIES (${MUPEN64_INCLUDE_DIRS})
INCLUDE_DIRECTORIES (${PROTOBUF_INCLUDE_DIRS})
INCLUDE_DIRECTORIES (${GRPC_INCLUDE_DIRS})

# We only need the GFlags library
FIND_LIBRARY (GFLAGS_LIBRARIES gflags)

# ------------------------------------------------------------------------------
# Declarations and subdirectories

# The proto base directory happens to be the same as the project root, but will 
# not necessarily remain so.
SET (PROTO_BASE_DIR ${CMAKE_SOURCE_DIR})

# Creates generated proto outputs and defines all required libraries
ADD_SUBDIRECTORY (base)

# Builds the client plugin
ADD_SUBDIRECTORY (client)

# TODO(alex): Move the integration tests into their own repository
# # Adds integration tests
# ADD_SUBDIRECTORY (integration_tests)

# Adds scripts
ADD_SUBDIRECTORY (scripts)
