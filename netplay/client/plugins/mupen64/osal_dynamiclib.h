#ifndef PLUGINS_MUPEN64_OSAL_DYNAMICLIB_H_
#define PLUGINS_MUPEN64_OSAL_DYNAMICLIB_H_

#include "m64p_types.h"

extern "C" {

void *osal_dynlib_getproc(m64p_dynlib_handle LibHandle,
                          const char *pccProcedureName);

}  // extern "C"

#endif  // PLUGINS_MUPEN64_OSAL_DYNAMICLIB_H_
