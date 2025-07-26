#pragma once

#include "../../../ESPEasy_common.h"

#include "../../../src/CustomBuild/ESPEasyLimits.h"
#include "../../../src/DataStructs/ESPEasy_EventStruct.h"
#include "../../../src/DataTypes/ESPEasy_plugin_functions.h"
#include "../DataTypes/NWPluginID.h"
#include "../DataTypes/NetworkIndex.h"
#include "../DataTypes/NetworkDriverIndex.h"

namespace ESPEasy {
namespace net {

bool NWPluginCall(NWPlugin::Function Function,
                  EventStruct       *event);
bool NWPluginCall(NWPlugin::Function Function,
                  EventStruct       *event,
                  String           & str);


bool validNetworkDriverIndex(networkDriverIndex_t index);

// bool getIP(networkDriverIndex_t index, NWPlugin::IP_type ip_type);


// bool              validNetworkIndex(networkIndex_t index);
#define validNetworkIndex(C_X) ((C_X) < NETWORK_MAX)

// Check whether NWPlugin is included in build.
bool validNWPluginID(nwpluginID_t nwpluginID);

// Check if nwplugin is included in build.
// N.B. Invalid nwplugin is also not considered supported.
// This is essentially (validNWPluginID && validNetworkDriverIndex)
bool                 supportedNWPluginID(nwpluginID_t nwpluginID);
networkDriverIndex_t getNetworkDriverIndex_from_NetworkIndex(networkIndex_t index);
networkDriverIndex_t getNetworkDriverIndex_from_NWPluginID(nwpluginID_t nwpluginID);
nwpluginID_t         getNWPluginID_from_NetworkDriverIndex(networkDriverIndex_t index);
nwpluginID_t         getNWPluginID_from_NetworkIndex(networkIndex_t index);

String               getNWPluginNameFromNetworkDriverIndex(networkDriverIndex_t NetworkDriverIndex);
String               getNWPluginNameFromNWPluginID(nwpluginID_t nwpluginID);

} // namespace net
} // namespace ESPEasy
