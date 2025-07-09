#include "ESPEasy_common.h"

#ifdef USES_NW002

// #######################################################################################################
// ########################### Network Plugin 002: WiFi Access Point #####################################
// #######################################################################################################

# define NWPLUGIN_002
# define NWPLUGIN_ID_002         2
# define NWPLUGIN_NAME_002       "WiFi AP"

# include "src/ESPEasyCore/ESPEasyNetwork.h"
# include "src/DataStructs/ESPEasy_EventStruct.h"
# include "src/Globals/NWPlugins.h"
# include "src/Helpers/ESPEasy_Storage.h"
# include "src/Helpers/_NWPlugin_init.h"

# include "src/Globals/SecuritySettings.h"
# include "src/WebServer/common.h"
# include "src/Helpers/StringConverter.h"
# include "src/WebServer/ESPEasy_WebServer.h"
# include "src/Globals/Settings.h"
# include "src/WebServer/Markup_Forms.h"
# include "src/WebServer/Markup.h"
# include "src/ESPEasyCore/ESPEasyWifi_abstracted.h"

bool NWPlugin_002(NWPlugin::Function function, struct EventStruct *event, String& string)
{
  bool success = false;

  switch (function)
  {
    case NWPlugin::Function::NWPLUGIN_DRIVER_ADD:
    {
      NetworkDriverStruct& nw = getNetworkDriverStruct(networkDriverIndex_t::toNetworkDriverIndex(event->idx));
      break;
    }

    case NWPlugin::Function::NWPLUGIN_GET_DEVICENAME:
    {
      string = F(NWPLUGIN_NAME_002);
      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_CONNECTED:
    {
# ifdef ESP32
      const uint8_t num = WiFi.AP.stationCount();
# else
      const uint8_t num = WiFi.softAPgetStationNum();
# endif // ifdef ESP32

      if (num > 0) {
        success = true;
        string  = concat(num, F(" client"));

        if (num > 1) { string += 's'; }
      }
      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_HOSTNAME:
    {
# ifdef ESP32
      string = WiFi.AP.getHostname();
# else
      string = WiFi.softAPSSID();
# endif // ifdef ESP32
      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_MAC:
    {
# ifdef ESP32
      string = WiFi.AP.macAddress();
# else
      string = WiFi.softAPmacAddress();
# endif // ifdef ESP32
      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_IP:
    {
# ifdef ESP32
      string = WiFi.AP.localIP().toString();
# else
      string = WiFi.softAPIP().toString();
# endif // ifdef ESP32
      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_PORT:
    {
      break;
    }


    case NWPlugin::Function::NWPLUGIN_WEBFORM_SAVE:
    {

      // Access point password.
      copyFormPassword(F("apkey"), SecuritySettings.WifiAPKey, sizeof(SecuritySettings.WifiAPKey));

      // When set you can use the Sensor in AP-Mode without being forced to /setup
      Settings.ApDontForceSetup(isFormItemChecked(F("ApDontForceSetup")));

      // Usually the AP will be started when no WiFi is defined, or the defined one cannot be found. This flag may prevent it.
      Settings.DoNotStartAP(isFormItemChecked(F("DoNotStartAP")));


      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_LOAD:
    {
      addFormPasswordBox(F("WPA AP Mode Key"), F("apkey"), SecuritySettings.WifiAPKey, 63);
      addFormNote(F("WPA Key must be at least 8 characters long"));

      addFormCheckBox(F("Don't force /setup in AP-Mode"), F("ApDontForceSetup"), Settings.ApDontForceSetup());
      addFormNote(F("When set you can use the Sensor in AP-Mode without being forced to /setup. /setup can still be called."));

      addFormCheckBox(F("Do Not Start AP"), F("DoNotStartAP"), Settings.DoNotStartAP());
  # if FEATURE_ETHERNET
      addFormNote(F("Do not allow to start an AP when unable to connect to configured LAN/WiFi"));
  # else // if FEATURE_ETHERNET
      addFormNote(F("Do not allow to start an AP when configured WiFi cannot be found"));
  # endif // if FEATURE_ETHERNET


      break;
    }

    case NWPlugin::Function::NWPLUGIN_INIT:
    {
      break;
    }


    default:
      break;

  }
  return success;
}

#endif // ifdef USES_NW002
