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




#endif




