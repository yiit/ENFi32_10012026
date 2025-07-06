#include "../Globals/NWPlugins.h"

#include "../../_Plugin_Helper.h"
#include "../DataStructs/ESPEasy_EventStruct.h"
#include "../DataStructs/TimingStats.h"
#include "../DataTypes/ESPEasy_plugin_functions.h"
#include "../ESPEasyCore/ESPEasy_Log.h"

#include "../Globals/Settings.h"
#include "../Helpers/_NWPlugin_init.h"

bool NWPluginCall(NWPlugin::Function Function, struct EventStruct *event) {
  #ifdef USE_SECOND_HEAP
  HeapSelectDram ephemeral;
  #endif // ifdef USE_SECOND_HEAP

  String dummy;

  return NWPluginCall(Function, event, dummy);
}

bool NWPluginCall(NWPlugin::Function Function, struct EventStruct *event, String& str)
{
  #ifdef USE_SECOND_HEAP
  HeapSelectDram ephemeral;
  #endif // ifdef USE_SECOND_HEAP

  struct EventStruct TempEvent;

  if (event == 0) {
    event = &TempEvent;
  }

  switch (Function)
  {
    case NWPlugin::Function::NWPLUGIN_ADAPTER_ADD:
      // only called from NWPluginSetup() directly using networkAdapterIndex
      break;

    // calls to all active networks
    case NWPlugin::Function::NWPLUGIN_INIT_ALL:
    case NWPlugin::Function::NWPLUGIN_TEN_PER_SECOND:
    case NWPlugin::Function::NWPLUGIN_FIFTY_PER_SECOND:
    case NWPlugin::Function::NWPLUGIN_WRITE:
    {
      const bool success = Function != NWPlugin::Function::NWPLUGIN_WRITE;

      if (Function == NWPlugin::Function::NWPLUGIN_INIT_ALL) {
        Function = NWPlugin::Function::NWPLUGIN_INIT;
      }

      // TODO TD-er: Implement

      return success;
    }

  }
  return false;
}

bool validNetworkAdapterIndex(networkAdapterIndex_t index) { return validNetworkAdapterIndex_init(index); }

/*
   bool validNetworkIndex(networkIndex_t index)
   {
   return index < NETWORK_MAX;
   }
 */
bool validNWPluginID(nwpluginID_t nwpluginID) { 
  return getNetworkAdapterIndex_from_NWPluginID_(nwpluginID) !=
         INVALID_NETWORKADAPTER_INDEX;
}

bool supportedNWPluginID(nwpluginID_t nwpluginID) { 
  return validNetworkAdapterIndex(
    getNetworkAdapterIndex_from_NWPluginID_(nwpluginID));
}

networkAdapterIndex_t getNetworkAdapterIndex_from_NetworkIndex(networkIndex_t index) {
  if (validNetworkIndex(index)) {
    return getNetworkAdapterIndex_from_NWPluginID_(Settings.getNWPluginID_for_network(index));
  }
  return INVALID_NETWORKADAPTER_INDEX;
}

networkAdapterIndex_t getNetworkAdapterIndex_from_NWPluginID(nwpluginID_t nwpluginID) {
  return getNetworkAdapterIndex_from_NWPluginID_(nwpluginID);
}

nwpluginID_t getNWPluginID_from_NetworkAdapterIndex(networkAdapterIndex_t index) { return getNWPluginID_from_NetworkAdapterIndex(index); }

nwpluginID_t getNWPluginID_from_NetworkIndex(networkIndex_t index) {
  const networkAdapterIndex_t networkAdapterIndex = getNetworkAdapterIndex_from_NetworkIndex(index);

  return getNWPluginID_from_NetworkAdapterIndex(networkAdapterIndex);
}

String getNWPluginNameFromNetworkAdapterIndex(networkAdapterIndex_t NetworkAdapterIndex) {
  String networkName;

  if (validNetworkAdapterIndex(NetworkAdapterIndex)) {
    NWPluginCall(NetworkAdapterIndex, NWPlugin::Function::NWPLUGIN_GET_DEVICENAME, nullptr, networkName);
  }
  return networkName;
}

String getNWPluginNameFromNWPluginID(nwpluginID_t nwpluginID) {
  networkAdapterIndex_t networkAdapterIndex = getNetworkAdapterIndex_from_NWPluginID(nwpluginID);

  if (!validNetworkAdapterIndex(networkAdapterIndex)) {
    return strformat(F("NWPlugin %d not included in build"), nwpluginID);
  }
  return getNWPluginNameFromNetworkAdapterIndex(networkAdapterIndex);
}
