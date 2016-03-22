#include "gtest/gtest.h"

#define TRACED_CALL(__CALL__) \
{ SCOPED_TRACE(""); \
  {__CALL__;}}
