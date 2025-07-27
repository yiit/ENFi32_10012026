#include "../../ESPEasy_common.h"

#ifdef USES_NW002

// #######################################################################################################
// ########################### Network Plugin 002: WiFi Access Point #####################################
// #######################################################################################################

# define NWPLUGIN_002
# define NWPLUGIN_ID_002         2
# define NWPLUGIN_NAME_002       "WiFi AP"

# include "../../src/DataStructs/ESPEasy_EventStruct.h"
# include "../../src/Globals/SecuritySettings.h"
# include "../../src/Globals/Settings.h"
# include "../../src/Helpers/ESPEasy_Storage.h"
# include "../../src/Helpers/PrintToString.h"
# include "../../src/Helpers/StringConverter.h"
# include "../../src/WebServer/ESPEasy_WebServer.h"
# include "../../src/WebServer/HTML_Print.h"
# include "../../src/WebServer/HTML_wrappers.h"
# include "../../src/WebServer/Markup.h"
# include "../../src/WebServer/Markup_Forms.h"
# include "../../src/WebServer/common.h"
# include "../net/ESPEasyNetwork.h"
# include "../net/Globals/NWPlugins.h"
# include "../net/Helpers/_NWPlugin_Helper_webform.h"
# include "../net/Helpers/_NWPlugin_init.h"



// TODO TD-er: This code should be moved to this NW002 plugin
# include "../net/wifi/ESPEasyWifi.h"

namespace ESPEasy {
namespace net {

bool NWPlugin_002(NWPlugin::Function function, EventStruct *event, String& string)
{
  bool success = false;

  switch (function)
  {
    case NWPlugin::Function::NWPLUGIN_DRIVER_ADD:
    {
      NetworkDriverStruct& nw = getNetworkDriverStruct(networkDriverIndex_t::toNetworkDriverIndex(event->idx));
      nw.onlySingleInstance = true;
      nw.alwaysPresent      = true;
      nw.fixedNetworkIndex  = NWPLUGIN_ID_002 - 1; // Start counting at 0
      break;
    }

    case NWPlugin::Function::NWPLUGIN_GET_DEVICENAME:
    {
      string = F(NWPLUGIN_NAME_002);
      break;
    }

# ifdef ESP32
    case NWPlugin::Function::NWPLUGIN_GET_INTERFACE:
    {
      event->networkInterface = &WiFi.AP;
      success                 = event->networkInterface != nullptr;
      break;
    }
# endif // ifdef ESP32


    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_CONNECTED:
    {
# ifdef ESP32
      const uint8_t num = WiFi.AP.stationCount();
# else
      const uint8_t num = WiFi.softAPgetStationNum();
# endif // ifdef ESP32

      if (num > 0) {
        success = true;
        string  = num;
        string += F(" client");

        if (num > 1) { string += 's'; }
      }
      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_HOSTNAME:
    {
# ifdef ESP32
      string = WiFi.AP.SSID();
# else
      string = WiFi.softAPSSID();
# endif // ifdef ESP32
      success = true;
      break;
    }

# ifdef ESP8266
    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_HW_ADDRESS:
    {
      string  = WiFi.softAPmacAddress();
      success = true;
      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_IP:
    {
      string  = WiFi.softAPIP().toString();
      success = true;
      break;
    }
# endif // ifdef ESP8266

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

# ifdef ESP32
      Settings.WiFi_AP_enable_NAPT(isFormItemChecked(F("napt")));
# endif

      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_LOAD:
    {
      addFormSubHeader(F("Wifi AP Settings"));
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
  # ifdef ESP32
      addFormCheckBox(F("Enable NAPT"), F("napt"), Settings.WiFi_AP_enable_NAPT());
  # endif

      break;
    }

    case NWPlugin::Function::NWPLUGIN_INIT:
    {
      ESPEasy::net::wifi::setAPinternal(true);

      //      WiFi.AP.begin();
      break;
    }

    case NWPlugin::Function::NWPLUGIN_EXIT:
    {
# ifdef ESP32
      WiFi.AP.end();
# endif
# ifdef ESP8266

      // WiFi.softAP
# endif // ifdef ESP8266
      break;
    }

    default:
      break;

  }
  return success;
}

} // namespace net
} // namespace ESPEasy

#endif // ifdef USES_NW002
