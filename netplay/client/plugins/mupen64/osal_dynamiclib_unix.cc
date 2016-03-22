#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#include "m64p_types.h"

#include "client/plugins/mupen64/osal_dynamiclib.h"

extern "C" {

void *osal_dynlib_getproc(m64p_dynlib_handle LibHandle,
                          const char *pccProcedureName) {
    if (pccProcedureName == NULL)
        return NULL;

    return dlsym(LibHandle, pccProcedureName);
}

}  // extern "C"
