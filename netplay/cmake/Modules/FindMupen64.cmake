# Locate the Mupen64Plus emulator API headers
#
# Defines the following variables:
#
#   MUPEN64_FOUND - Found the Mupen64Plus API headers
#   MUPEN64_INCLUDE_DIRS - Include directories
#
# Accepts the following variables as input:
#
#   MUPEN64_ROOT - (as a CMake or environment variable)
#                  The root directory of the Mupen64Plus git repository prefix

IF (DEFINED ENV{MUPEN64_ROOT})
  SET (MUPEN64_HINTS ${MUPEN64_HINTS} $ENV{MUPEN64_ROOT})
ENDIF()
IF (DEFINED MUPEN64_ROOT)
  SET (MUPEN64_HINTS ${MUPEN64_HINTS} ${MUPEN64_ROOT})
ENDIF()
IF (NOT DEFINED MUPEN64_HINTS) 
  SET (MUPEN64_HINTS ${CMAKE_SOURCE_DIR}/mupen64plus-core/src/api)
ENDIF()

FIND_PATH (
  MUPEN64_INCLUDE_DIR
  NAMES
    m64p_common.h
    m64p_config.h
    m64p_plugin.h
  HINTS
    ${MUPEN64_HINTS}
  DOC "Mupen64 API header directory"
)
MARK_AS_ADVANCED (MUPEN64_INCLUDE_DIR)

INCLUDE (FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Mupen64 DEFAULT_MSG MUPEN64_INCLUDE_DIR)

MESSAGE (STATUS "Using Mupen64 API headers at ${MUPEN64_INCLUDE_DIR}")

IF (MUPEN64_FOUND)
  SET (MUPEN64_INCLUDE_DIRS ${MUPEN64_INCLUDE_DIR})
ENDIF()
