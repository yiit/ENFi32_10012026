#ifndef HELPERS__NWPLUGIN_HELPER_WEBFORM_H
#define HELPERS__NWPLUGIN_HELPER_WEBFORM_H

#include "../../ESPEasy_common.h"

#include "../DataStructs/NetworkSettingsStruct.h"
#include "../Globals/NWPlugins.h"


/*********************************************************************************************\
* Functions to load and store network settings on the web page.
\*********************************************************************************************/
String getNetworkParameterName(networkDriverIndex_t           NetworkDriverIndex,
                               NetworkSettingsStruct::VarType parameterIdx,
                               bool                           displayName,
                               bool                         & isAlternative);

String getNetworkParameterInternalName(networkDriverIndex_t           NetworkDriverIndex,
                                       NetworkSettingsStruct::VarType parameterIdx);

String getNetworkParameterDisplayName(networkDriverIndex_t           NetworkDriverIndex,
                                      NetworkSettingsStruct::VarType parameterIdx,
                                      bool                         & isAlternative);

void addNetworkEnabledForm(networkIndex_t networkindex);

void addNetworkParameterForm(const NetworkSettingsStruct  & NetworkSettings,
                             networkIndex_t                 networkindex,
                             NetworkSettingsStruct::VarType varType);

void saveNetworkParameterForm(NetworkSettingsStruct        & NetworkSettings,
                              networkIndex_t                 networkindex,
                              NetworkSettingsStruct::VarType varType);


#endif // ifndef HELPERS__NWPLUGIN_HELPER_WEBFORM_H
