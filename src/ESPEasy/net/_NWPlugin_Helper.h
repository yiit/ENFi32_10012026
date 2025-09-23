#pragma once

#include "../../ESPEasy_common.h"

#include "../../src/CustomBuild/ESPEasyLimits.h"
#include "../../src/DataStructs/ESPEasy_EventStruct.h"
#include "../../src/Helpers/LongTermTimer.h"
#include "../net/DataStructs/NWPluginData_base.h"
#include "../net/ESPEasyNetwork.h"

namespace ESPEasy {
namespace net {


extern NWPluginData_base *NWPlugin_task_data[NETWORK_MAX];

// Try to allocate in PSRAM or 2nd heap if possible
#define special_initNWPluginData(I, T) void *ptr = special_calloc(1, sizeof(T)); \
        if (ptr) { initNWPluginData(I, new (ptr) T()); }


// ==============================================
// Data used by instances of NWPlugins.
// =============================================

void resetNWPluginData();

void clearNWPluginData(ESPEasy::net::networkIndex_t networkIndex);

bool initNWPluginData(ESPEasy::net::networkIndex_t     networkIndex,
                      NWPluginData_base * data);

NWPluginData_base* getNWPluginData(ESPEasy::net::networkIndex_t networkIndex);
NWPluginData_base* getNWPluginDataBaseClassOnly(ESPEasy::net::networkIndex_t networkIndex);

bool nwpluginTaskData_initialized(ESPEasy::net::networkIndex_t networkIndex);

} // namespace net
} // namespace ESPEasy
