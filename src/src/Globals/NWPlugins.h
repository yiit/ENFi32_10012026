#ifndef GLOBALS_NWPLUGIN_H
#define GLOBALS_NWPLUGIN_H

#include "../../ESPEasy_common.h"

#include "../CustomBuild/ESPEasyLimits.h"
#include "../DataTypes/ESPEasy_plugin_functions.h"
#include "../DataTypes/NWPluginID.h"
#include "../DataTypes/NetworkIndex.h"
#include "../DataTypes/NetworkAdapterIndex.h"





bool NWPluginCall(NWPlugin::Function   Function,
                 struct EventStruct *event);
bool NWPluginCall(NWPlugin::Function   Function,
                 struct EventStruct *event,
                 String            & str);
bool NWPluginCall(networkAdapterIndex_t     networkAdapterIndex,
                 NWPlugin::Function   Function,
                 struct EventStruct *event,
                 String            & str);


                 
bool              validNetworkAdapterIndex(networkAdapterIndex_t index);


// bool              validNetworkIndex(networkIndex_t index);
#define validNetworkIndex(C_X)  ((C_X) < NETWORK_MAX)

// Check whether NWPlugin is included in build.
bool              validNWPluginID(nwpluginID_t nwpluginID);

// Check if nwplugin is included in build.
// N.B. Invalid nwplugin is also not considered supported.
// This is essentially (validNWPluginID && validNetworkAdapterIndex)
bool            supportedNWPluginID(nwpluginID_t nwpluginID);
networkAdapterIndex_t getNetworkAdapterIndex_from_NetworkIndex(networkIndex_t index);
networkAdapterIndex_t getNetworkAdapterIndex_from_NWPluginID(nwpluginID_t nwpluginID);
nwpluginID_t     getNWPluginID_from_NetworkAdapterIndex(networkAdapterIndex_t index);
nwpluginID_t     getNWPluginID_from_NetworkIndex(networkIndex_t index);

String          getNWPluginNameFromNetworkAdapterIndex(networkAdapterIndex_t NetworkAdapterIndex);
String          getNWPluginNameFromNWPluginID(nwpluginID_t nwpluginID);


#endif




