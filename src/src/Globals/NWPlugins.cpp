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
      // only called from NWPluginSetup() directly using protocolIndex
      break;

    // calls to all active controllers
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