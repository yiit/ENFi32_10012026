#include "../Globals/NWPlugins.h"

#include "../../../_Plugin_Helper.h"
#include "../../../src/DataStructs/ESPEasy_EventStruct.h"
#include "../../../src/DataStructs/TimingStats.h"
#include "../../../src/DataTypes/ESPEasy_plugin_functions.h"
#include "../../../src/ESPEasyCore/ESPEasy_Log.h"
#include "../../../src/Globals/Settings.h"
#include "../../../src/Helpers/PrintToString.h"
#include "../Helpers/_NWPlugin_init.h"
#include "../_NWPlugin_Helper.h"

namespace ESPEasy {
namespace net {

#define NETWORK_INDEX_WIFI_STA  0 // Always the first network index

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

    case NWPlugin::Function::NWPLUGIN_GET_PARAMETER_DISPLAY_NAME:
      // Only called from _NWPlugin_Helper_webform  getNetworkParameterName
      break;

    // calls to all active networks
    case NWPlugin::Function::NWPLUGIN_INIT_ALL:
    case NWPlugin::Function::NWPLUGIN_EXIT_ALL:
    case NWPlugin::Function::NWPLUGIN_TEN_PER_SECOND:
    case NWPlugin::Function::NWPLUGIN_FIFTY_PER_SECOND:
    case NWPlugin::Function::NWPLUGIN_WRITE:
#ifdef ESP32
    case NWPlugin::Function::NWPLUGIN_PRIORITY_ROUTE_CHANGED:
#endif
    {
      // Set to true where return value doesn't matter
      bool success =
#ifdef ESP32
        Function != NWPlugin::Function::NWPLUGIN_PRIORITY_ROUTE_CHANGED &&
#endif
        Function != NWPlugin::Function::NWPLUGIN_WRITE;

      if (Function == NWPlugin::Function::NWPLUGIN_INIT_ALL) {
        Function = NWPlugin::Function::NWPLUGIN_INIT;
      }

      if (Function == NWPlugin::Function::NWPLUGIN_EXIT_ALL) {
        Function = NWPlugin::Function::NWPLUGIN_EXIT;
      }

      for (networkIndex_t x = 0; x < NETWORK_MAX; x++) {
        const bool checkedEnabled =
          Settings.getNetworkEnabled(x) ||
          Function == NWPlugin::Function::NWPLUGIN_EXIT;

        if (Settings.getNWPluginID_for_network(x) && checkedEnabled) {
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
              // Need to stop when call was handled
              return true;
            }
            success = true;
          }
        }
      }
      return success;
    }

      // calls to specific network's NWPluginData_base (must be enabled)
#ifdef ESP32
    case NWPlugin::Function::NWPLUGIN_GET_TRAFFIC_COUNT:
#endif // ifdef ESP32
#if FEATURE_PLUGIN_STATS
    case NWPlugin::Function::NWPLUGIN_RECORD_STATS:
    case NWPlugin::Function::NWPLUGIN_WEBFORM_LOAD_SHOW_STATS:
#endif // if FEATURE_PLUGIN_STATS
    case NWPlugin::Function::NWPLUGIN_GET_CONNECTED_DURATION:
    {
      bool success = false;

      if (!validNetworkIndex(event->NetworkIndex) ||
          !Settings.getNetworkEnabled(event->NetworkIndex)) { return false; }

      auto *NW_data = getNWPluginData(event->NetworkIndex);

      if (NW_data) {
        switch (Function)
        {
#ifdef ESP32
          case NWPlugin::Function::NWPLUGIN_GET_TRAFFIC_COUNT:
          {
            uint64_t tx{};
            uint64_t rx{};

            if (NW_data->getTrafficCount(tx, rx)) {
              event->Par64_1 = tx;
              event->Par64_2 = rx;
              success        = true;
            }
            break;
          }
#endif // ifdef ESP32
#if FEATURE_PLUGIN_STATS
          case NWPlugin::Function::NWPLUGIN_RECORD_STATS:
          {
//            NWPluginData_static_runtime& runtime_data = NW_data->getNWPluginData_static_runtime();
            success = NW_data->record_stats();
            break;
          }
          case NWPlugin::Function::NWPLUGIN_WEBFORM_LOAD_SHOW_STATS:
          {
//            NWPluginData_static_runtime& runtime_data = NW_data->getNWPluginData_static_runtime();
            success = NW_data->webformLoad_show_stats(event);
            break;
          }
#endif // if FEATURE_PLUGIN_STATS

          case NWPlugin::Function::NWPLUGIN_GET_CONNECTED_DURATION:
          {
            NWPluginData_static_runtime& runtime_data = NW_data->getNWPluginData_static_runtime();
            auto duration                             = runtime_data._connectedStats.getLastOnDuration_ms();

            if (duration > 0) {
              str            = format_msec_duration_HMS(duration);
              event->Par64_1 = duration;
              event->Par64_2 = runtime_data._connectedStats.getCycleCount();
              success        = true;
            }
            break;
          }
          default: break;
        }
      }

      return success;
    }


    // calls to specific network which need to be enabled before calling
    case NWPlugin::Function::NWPLUGIN_INIT:
    case NWPlugin::Function::NWPLUGIN_CONNECT_SUCCESS:
    case NWPlugin::Function::NWPLUGIN_CONNECT_FAIL:
    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_CONNECTED:
    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_HW_ADDRESS:
    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_IP:
    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_PORT:
    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_HOSTNAME:
#ifdef ESP32
    case NWPlugin::Function::NWPLUGIN_GET_INTERFACE:
    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_ROUTE_PRIO:
#endif // ifdef ESP32

      if (!validNetworkIndex(event->NetworkIndex) ||
          !Settings.getNetworkEnabled(event->NetworkIndex)) { return false; }

    // fall through

    // calls to specific network
    case NWPlugin::Function::NWPLUGIN_LOAD_DEFAULTS:
    case NWPlugin::Function::NWPLUGIN_EXIT:
    case NWPlugin::Function::NWPLUGIN_GET_DEVICENAME:
    case NWPlugin::Function::NWPLUGIN_WEBFORM_LOAD:
    case NWPlugin::Function::NWPLUGIN_WEBFORM_SAVE:
    case NWPlugin::Function::NWPLUGIN_DRIVER_TEMPLATE:
    {
      const networkIndex_t networkIndex = event->NetworkIndex;
      bool success                      = false;

      if (validNetworkIndex(networkIndex)) {
        if (supportedNWPluginID(Settings.getNWPluginID_for_network(networkIndex)))
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
# if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 4, 2)

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

        if (success && (Function == NWPlugin::Function::NWPLUGIN_EXIT)) {
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

NWPluginData_static_runtime* getWiFi_STA_NWPluginData_static_runtime()
{
  auto NW_data = getNWPluginData(NETWORK_INDEX_WIFI_STA);

  if (!NW_data) { return nullptr; }
  return &NW_data->getNWPluginData_static_runtime();
}

NWPluginData_static_runtime* getNWPluginData_static_runtime(networkIndex_t index)
{
  auto NW_data = getNWPluginData(index);

  if (!NW_data) { return nullptr; }
  return &NW_data->getNWPluginData_static_runtime();
}

const NWPluginData_static_runtime* getDefaultRoute_NWPluginData_static_runtime()
{
  for (networkIndex_t i = 0; validNetworkIndex(i); ++i) {
    auto NW_data = getNWPluginData_static_runtime(i);

    if (NW_data && NW_data->isDefaultRoute()) { return NW_data; }
  }
  return nullptr;
}

} // namespace net
} // namespace ESPEasy
