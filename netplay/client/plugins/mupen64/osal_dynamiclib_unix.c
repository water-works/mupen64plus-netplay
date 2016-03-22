#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#include "m64p_types.h"
#include "osal_dynamiclib.h"

void *osal_dynlib_getproc(m64p_dynlib_handle LibHandle,
                          const char *pccProcedureName) {
    if (pccProcedureName == NULL)
        return NULL;

    return dlsym(LibHandle, pccProcedureName);
}
