#include "../Globals/NWPlugins.h"

#include "../../_Plugin_Helper.h"
#include "../DataStructs/ESPEasy_EventStruct.h"
#include "../DataStructs/TimingStats.h"
#include "../DataTypes/ESPEasy_plugin_functions.h"
#include "../ESPEasyCore/ESPEasy_Log.h"

#include "../Globals/Settings.h"
#include "../Globals/NWPlugins.h"
#include "../Helpers/_NWPlugin_init.h"
#include "../Helpers/PrintToString.h"

namespace ESPEasy {
namespace net {

bool NWPluginCall(NWPlugin::Function Function, EventStruct *event) {
  #ifdef USE_SECOND_HEAP
  HeapSelectDram ephemeral;
  #endif // ifdef USE_SECOND_HEAP

  String dummy;

  return NWPluginCall(Function, event, dummy);
}

bool NWPluginCall(NWPlugin::Function Function, EventStruct *event, String& str)
{
  #ifdef USE_SECOND_HEAP
  HeapSelectDram ephemeral;
  #endif // ifdef USE_SECOND_HEAP

  EventStruct TempEvent;

  if (event == 0) {
    event = &TempEvent;
  }

  switch (Function)
  {
    case NWPlugin::Function::NWPLUGIN_DRIVER_ADD:
      // only called from NWPluginSetup() directly using networkDriverIndex
      break;

    // calls to all active networks
    case NWPlugin::Function::NWPLUGIN_INIT_ALL:
    case NWPlugin::Function::NWPLUGIN_EXIT_ALL:
    case NWPlugin::Function::NWPLUGIN_TEN_PER_SECOND:
    case NWPlugin::Function::NWPLUGIN_FIFTY_PER_SECOND:
    case NWPlugin::Function::NWPLUGIN_WRITE:
    {
      const bool success = Function != NWPlugin::Function::NWPLUGIN_WRITE;

      if (Function == NWPlugin::Function::NWPLUGIN_INIT_ALL) {
        Function = NWPlugin::Function::NWPLUGIN_INIT;
      }

      if (Function == NWPlugin::Function::NWPLUGIN_EXIT_ALL) {
        Function = NWPlugin::Function::NWPLUGIN_EXIT;
      }

      for (networkIndex_t x = 0; x < NETWORK_MAX; x++) {
        if (Settings.getNWPluginID_for_network(x) && Settings.getNetworkEnabled(x)) {
          event->NetworkIndex = x;
          String command;

          if (Function == NWPlugin::Function::NWPLUGIN_WRITE) {
            command = str;
          }

          if (do_NWPluginCall(
                getNetworkDriverIndex_from_NetworkIndex(x),
                Function,
                event,
                command)) {
            if (Function == NWPlugin::Function::NWPLUGIN_WRITE) {
              // Need to stop when write call was handled
              return true;
            }
          }
        }
      }
      break;
    }

    // calls to specific network
    case NWPlugin::Function::NWPLUGIN_LOAD_DEFAULTS:
    case NWPlugin::Function::NWPLUGIN_INIT:
    case NWPlugin::Function::NWPLUGIN_EXIT:
    case NWPlugin::Function::NWPLUGIN_GET_DEVICENAME:
    case NWPlugin::Function::NWPLUGIN_WEBFORM_LOAD:
    case NWPlugin::Function::NWPLUGIN_WEBFORM_SAVE:
#ifdef ESP32
    case NWPlugin::Function::NWPLUGIN_GET_INTERFACE:
    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_ROUTE_PRIO:
#endif // ifdef ESP32
    case NWPlugin::Function::NWPLUGIN_CONNECT_SUCCESS:
    case NWPlugin::Function::NWPLUGIN_CONNECT_FAIL:
    case NWPlugin::Function::NWPLUGIN_DRIVER_TEMPLATE:
    case NWPlugin::Function::NWPLUGIN_GET_PARAMETER_DISPLAY_NAME:
    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_CONNECTED:
    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_HOSTNAME:
    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_HW_ADDRESS:
    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_IP:
    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_PORT:
    {
      const networkIndex_t networkIndex = event->NetworkIndex;
      bool success                      = false;

      if (validNetworkIndex(networkIndex)) {
        if (Settings.getNetworkEnabled(networkIndex) && supportedNWPluginID(Settings.getNWPluginID_for_network(networkIndex)))
        {
          const networkDriverIndex_t networkDriverIndex =
            getNetworkDriverIndex_from_NetworkIndex(networkIndex);

          success = do_NWPluginCall(
            networkDriverIndex,
            Function,
            event,
            str);
#ifdef ESP32

          if (!success && NWPlugin::canQueryViaNetworkInterface(Function)) {
            String dummy_str;

            if (do_NWPluginCall(
                  networkDriverIndex,
                  NWPlugin::Function::NWPLUGIN_GET_INTERFACE,
                  event,
                  dummy_str)) {
              if (event->networkInterface != nullptr) {
                switch (Function)
                {
                  case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_ROUTE_PRIO:
# if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0)

                    // TODO TD-er: Must also add option to set route prio
                    // See: https://github.com/espressif/arduino-esp32/pull/11617
                    event->Par1 = event->networkInterface->getRoutePrio();
# else // if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0)
                    event->Par1 = event->networkInterface->route_prio();
# endif // if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0)
                    event->Par2 = event->networkInterface->isDefault();
                    success     = true;
                    break;

                  case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_HOSTNAME:
                    str     = event->networkInterface->getHostname();
                    success = true;
                    break;

                  case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_HW_ADDRESS:
                    str            = event->networkInterface->macAddress();
                    event->String1 = F("MAC");
                    success        = true;
                    break;

                  case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_IP:
                  {
                    PrintToString prstr;
                    success = NWPlugin::print_IP_address(
                      static_cast<NWPlugin::IP_type>(event->Par1),
                      event->networkInterface,
                      prstr);
                    str = prstr.get();
                    break;
                  }
                  default: break;
                }
              }
            }
          }
#endif // ifdef ESP32
        }
#ifdef ESP32

        if (Function == NWPlugin::Function::NWPLUGIN_EXIT) {
          //          Cache.clearNetworkSettings(networkIndex);
        }
#endif // ifdef ESP32
      }
      return success;
    }

  }
  return false;
}

bool validNetworkDriverIndex(networkDriverIndex_t index) { return do_check_validNetworkDriverIndex(index); }


// bool getIP(networkDriverIndex_t index, NWPlugin::IP_type ip_type){}

/*
   bool validNetworkIndex(networkIndex_t index)
   {
   return index < NETWORK_MAX;
   }
 */
bool validNWPluginID(nwpluginID_t nwpluginID) {
  return do_getNetworkDriverIndex_from_NWPluginID(nwpluginID) !=
         INVALID_NETWORKDRIVER_INDEX;
}

bool supportedNWPluginID(nwpluginID_t nwpluginID) {
  return validNetworkDriverIndex(
    do_getNetworkDriverIndex_from_NWPluginID(nwpluginID));
}

networkDriverIndex_t getNetworkDriverIndex_from_NetworkIndex(networkIndex_t index) {
  if (validNetworkIndex(index)) {
    return do_getNetworkDriverIndex_from_NWPluginID(Settings.getNWPluginID_for_network(index));
  }
  return INVALID_NETWORKDRIVER_INDEX;
}

networkDriverIndex_t getNetworkDriverIndex_from_NWPluginID(nwpluginID_t nwpluginID) {
  return do_getNetworkDriverIndex_from_NWPluginID(nwpluginID);
}

nwpluginID_t getNWPluginID_from_NetworkDriverIndex(networkDriverIndex_t index)      { return do_getNWPluginID_from_NetworkDriverIndex(index);
}

nwpluginID_t getNWPluginID_from_NetworkIndex(networkIndex_t index) {
  const networkDriverIndex_t networkDriverIndex = getNetworkDriverIndex_from_NetworkIndex(index);

  return do_getNWPluginID_from_NetworkDriverIndex(networkDriverIndex);
}

String getNWPluginNameFromNetworkDriverIndex(networkDriverIndex_t NetworkDriverIndex) {
  String networkName;

  if (validNetworkDriverIndex(NetworkDriverIndex)) {
    do_NWPluginCall(NetworkDriverIndex, NWPlugin::Function::NWPLUGIN_GET_DEVICENAME, nullptr, networkName);
  }
  return networkName;
}

String getNWPluginNameFromNWPluginID(nwpluginID_t nwpluginID) {
  networkDriverIndex_t networkDriverIndex = getNetworkDriverIndex_from_NWPluginID(nwpluginID);

  if (!validNetworkDriverIndex(networkDriverIndex)) {
    return strformat(F("NWPlugin %d not included in build"), nwpluginID.value);
  }
  return getNWPluginNameFromNetworkDriverIndex(networkDriverIndex);
}

} // namespace net
} // namespace ESPEasy
