#ifndef MUPEN64_H_
#define MUPEN64_H_

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "m64p_plugin.h"
#include "m64p_types.h"

extern "C" {

#define MESSAGE_BUFFER_SIZE 2048
#define NETPLAY_API_VERSION 0x20000

// -----------------------------------------------------------------------------
// Global functions

// Log an error using the DebugMessage callback that was passed to 
// PluginStartup.
void DebugMessage(int level, const char *message, ...);

// -----------------------------------------------------------------------------
// Plugin methods. See the mupen64 documentation for more information.

// Initializes all plugin local data:
//  - Sets the debug message callback.
//  - Initializes the configuration API.
EXPORT m64p_error CALL
PluginStartup(m64p_dynlib_handle CoreLibHandle, void *Context,
              void (*DebugCallback)(void *, int, const char *));

// Returns all required version info.
EXPORT m64p_error CALL PluginGetVersion(m64p_plugin_type *plugin_type,
                                        int *plugin_version, int *api_version,
                                        const char **plugin_name, int *caps);

// Loads configuration data and connects to the server.
EXPORT int CALL RomOpen(void);

// Negotiates the connection to the server.
EXPORT int CALL InitiateNetplay(NETPLAY_INFO *netplay_info,
                                const char *goodname, const char *md5);

// Called when the ROM is closed.
// TODO(alexgolec): Cleanly close the netplay connection.
EXPORT void CALL RomClosed(void);

// Puts and retrieves button updates.
EXPORT
int CALL PutKeys(const m64p_netplay_frame_update *updates, int nupdates);
EXPORT
int CALL GetKeys(m64p_netplay_frame_update *update);

// Does nothing, only exists to satisfy the Python frontend.
EXPORT m64p_error PluginShutdown();

}  // extern "C"

#endif  // MUPEN64_H_
