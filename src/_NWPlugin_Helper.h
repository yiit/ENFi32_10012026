#ifndef _NWPLUGIN_HELPER_H
#define _NWPLUGIN_HELPER_H

#include "ESPEasy_common.h"

#if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS
# include "src/CustomBuild/ESPEasyLimits.h"


# include "src/DataStructs/NWPluginData_base.h"

extern NWPluginData_base *NWPlugin_task_data[NETWORK_MAX];

// Try to allocate in PSRAM or 2nd heap if possible
# define special_initNWPluginData(I, T) void *ptr = special_calloc(1, sizeof(T)); \
        if (ptr) { initNWPluginData(I, new (ptr) T()); }


// ==============================================
// Data used by instances of NWPlugins.
// =============================================

void               resetNWPluginData();

void               clearNWPluginData(networkIndex_t networkIndex);

bool               initNWPluginData(networkIndex_t     networkIndex,
                                    NWPluginData_base *data);

NWPluginData_base* getNWPluginData(networkIndex_t networkIndex);
NWPluginData_base* getNWPluginDataBaseClassOnly(networkIndex_t networkIndex);

bool               nwpluginTaskData_initialized(networkIndex_t networkIndex);

#endif // if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS
#endif // ifndef _NWPLUGIN_HELPER_H
