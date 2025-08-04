#include "../../ESPEasy_common.h"

#ifdef USES_NW001

// #######################################################################################################
// ########################### Network Plugin 001: WiFi Station ##########################################
// #######################################################################################################

# define NWPLUGIN_001
# define NWPLUGIN_ID_001         1
# define NWPLUGIN_NAME_001       "WiFi Station"

# include "../../src/DataStructs/ESPEasy_EventStruct.h"
# include "../../src/Globals/RTC.h"
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
# include "../net/Globals/ESPEasyWiFiEvent.h"
# include "../net/Globals/NWPlugins.h"
# include "../net/Globals/WiFi_AP_Candidates.h"
# include "../net/Helpers/_NWPlugin_Helper_webform.h"
# include "../net/Helpers/_NWPlugin_init.h"
# include "../net/NWPluginStructs/NW001_data_struct_WiFi_STA.h"
# include "../net/wifi/ESPEasyWifi.h"
# include "../net/wifi/ESPEasyWifi_ProcessEvent.h"


# ifdef ESP32P4
#  include <esp_hosted.h>
# endif

namespace ESPEasy {
namespace net {

bool NWPlugin_001(NWPlugin::Function function, EventStruct *event, String& string)
{
  bool success = false;

  switch (function)
  {
    case NWPlugin::Function::NWPLUGIN_DRIVER_ADD:
    {
      NetworkDriverStruct& nw = getNetworkDriverStruct(networkDriverIndex_t::toNetworkDriverIndex(event->idx));
      nw.onlySingleInstance = true;
      nw.alwaysPresent      = true;
      nw.fixedNetworkIndex  = NWPLUGIN_ID_001 - 1; // Start counting at 0
      break;
    }

    case NWPlugin::Function::NWPLUGIN_GET_DEVICENAME:
    {
      string = F(NWPLUGIN_NAME_001);
      break;
    }
# ifdef ESP32
    case NWPlugin::Function::NWPLUGIN_GET_INTERFACE:
    {
      event->networkInterface = &WiFi.STA;
      success                 = event->networkInterface != nullptr;
      break;
    }
# endif // ifdef ESP32

    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_CONNECTED:
    {
# ifdef ESP32
      success = WiFi.STA.connected();

      if (success) {
        string = strformat(F("%s (ch: %d)\n%s"),
                           WiFi.STA.SSID().c_str(),
                           WiFi.channel(),
                           WiFi.STA.BSSIDstr().c_str());
      }
# else // ifdef ESP32
      success = WiFi.isConnected();

      if (success) {
        string = strformat(F("%s (ch: %d)\n%s"),
                           WiFi.SSID().c_str(),
                           WiFi.channel(),
                           WiFi.BSSIDstr().c_str());
      }
# endif // ifdef ESP32

      if (success) {
        ESPEasy::net::wifi::NW001_data_struct_WiFi_STA *NW_data =
          static_cast<ESPEasy::net::wifi::NW001_data_struct_WiFi_STA *>(getNWPluginData(event->NetworkIndex));

        if (NW_data) {
          auto connectionDuration_ms = NW_data->getConnectedDuration_ms();

          if (connectionDuration_ms > 0) {
            string += concat(
              '\n',
              format_msec_duration_HMS(connectionDuration_ms));
          }
        }
      }

      break;
    }

# ifdef ESP8266
    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_HOSTNAME:
    {
      string  = WiFi.hostname();
      success = true;
      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_HW_ADDRESS:
    {
      string  = WiFi.macAddress();
      success = true;
      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_IP:
    {
      string  = WiFi.localIP().toString();
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
      // SSID 1
      safe_strncpy(SecuritySettings.WifiSSID, webArg(F("ssid")).c_str(), sizeof(SecuritySettings.WifiSSID));
      copyFormPassword(F("key"), SecuritySettings.WifiKey, sizeof(SecuritySettings.WifiKey));

      // SSID 2
      strncpy_webserver_arg(SecuritySettings.WifiSSID2, F("ssid2"));
      copyFormPassword(F("key2"), SecuritySettings.WifiKey2, sizeof(SecuritySettings.WifiKey2));

      // Hidden SSID
      Settings.IncludeHiddenSSID(isFormItemChecked(LabelType::CONNECT_HIDDEN_SSID));
      Settings.HiddenSSID_SlowConnectPerBSSID(isFormItemChecked(LabelType::HIDDEN_SSID_SLOW_CONNECT));

# ifdef ESP32
      Settings.PassiveWiFiScan(isFormItemChecked(LabelType::WIFI_PASSIVE_SCAN));
# endif

      webArg2ip(F("espip"),      Settings.IP);
      webArg2ip(F("espgateway"), Settings.Gateway);
      webArg2ip(F("espsubnet"),  Settings.Subnet);
      webArg2ip(F("espdns"),     Settings.DNS);


      Settings.ForceWiFi_bg_mode(isFormItemChecked(LabelType::FORCE_WIFI_BG));
      Settings.WiFiRestart_connection_lost(isFormItemChecked(LabelType::RESTART_WIFI_LOST_CONN));

      Settings.WifiNoneSleep(isFormItemChecked(LabelType::FORCE_WIFI_NOSLEEP));
# ifdef SUPPORT_ARP
      Settings.gratuitousARP(isFormItemChecked(LabelType::PERIODICAL_GRAT_ARP));
# endif // ifdef SUPPORT_ARP
# if FEATURE_SET_WIFI_TX_PWR
      Settings.setWiFi_TX_power(getFormItemFloat(LabelType::WIFI_TX_MAX_PWR));
      Settings.WiFi_sensitivity_margin = getFormItemInt(LabelType::WIFI_SENS_MARGIN);
      Settings.UseMaxTXpowerForSending(isFormItemChecked(LabelType::WIFI_SEND_AT_MAX_TX_PWR));
# endif // if FEATURE_SET_WIFI_TX_PWR
      Settings.NumberExtraWiFiScans = getFormItemInt(LabelType::WIFI_NR_EXTRA_SCANS);
      Settings.UseLastWiFiFromRTC(isFormItemChecked(LabelType::WIFI_USE_LAST_CONN_FROM_RTC));

# ifndef ESP32
      Settings.WaitWiFiConnect(isFormItemChecked(LabelType::WAIT_WIFI_CONNECT));
# endif
      Settings.SDK_WiFi_autoreconnect(isFormItemChecked(LabelType::SDK_WIFI_AUTORECONNECT));
# if CONFIG_SOC_WIFI_SUPPORT_5G
      Settings.WiFi_band_mode(static_cast<wifi_band_mode_t>(getFormItemInt(getInternalLabel(LabelType::WIFI_BAND_MODE))));
# endif

      /*
       # if FEATURE_USE_IPV6
            Settings.EnableIPv6(isFormItemChecked(LabelType::ENABLE_IPV6));
       # endif
       */

      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_LOAD:
    {
      addFormSubHeader(F("Wifi Settings"));

      // TODO Add pin configuration for ESP32P4.
      // ESP32-C5 may use different SDIO pins.
      // See: https://github.com/espressif/esp-hosted-mcu/blob/main/docs/sdio.md#esp32-p4-function-ev-board-host-pin-mapping


# ifdef ESP32P4
      {
        addRowLabel(F("ESP Hosted Firmware"));
        esp_hosted_coprocessor_fwver_t ver_info{};
        esp_hosted_get_coprocessor_fwversion(&ver_info);
        addHtml(strformat(
                  F("%d.%d.%d"),
                  ver_info.major1,
                  ver_info.minor1,
                  ver_info.patch1));
      }
# endif // ifdef ESP32P4


      addFormTextBox(getLabel(LabelType::SSID), F("ssid"), SecuritySettings.WifiSSID, 31);
      addFormPasswordBox(F("WPA Key"), F("key"), SecuritySettings.WifiKey, 63);
      addFormTextBox(F("Fallback SSID"), F("ssid2"), SecuritySettings.WifiSSID2, 31);
      addFormPasswordBox(F("Fallback WPA Key"), F("key2"), SecuritySettings.WifiKey2, 63);
      addFormNote(F("WPA Key must be at least 8 characters long"));

      addFormCheckBox(LabelType::CONNECT_HIDDEN_SSID,      Settings.IncludeHiddenSSID());
      addFormCheckBox(LabelType::HIDDEN_SSID_SLOW_CONNECT, Settings.HiddenSSID_SlowConnectPerBSSID());
# ifdef ESP32
      addFormCheckBox(LabelType::WIFI_PASSIVE_SCAN,        Settings.PassiveWiFiScan());
# endif

      addFormSubHeader(F("WiFi IP Settings"));

      addFormIPBox(F("ESP WiFi IP"),         F("espip"),      Settings.IP);
      addFormIPBox(F("ESP WiFi Gateway"),    F("espgateway"), Settings.Gateway);
      addFormIPBox(F("ESP WiFi Subnetmask"), F("espsubnet"),  Settings.Subnet);
      addFormIPBox(F("ESP WiFi DNS"),        F("espdns"),     Settings.DNS);
      addFormNote(F("Leave empty for DHCP"));

      /*
       # if FEATURE_USE_IPV6
            addFormCheckBox(LabelType::ENABLE_IPV6, Settings.EnableIPv6());
       # endif
       */

      addFormSubHeader(F("WiFi Mode"));
      addFormCheckBox(LabelType::FORCE_WIFI_BG, Settings.ForceWiFi_bg_mode());
# if CONFIG_SOC_WIFI_SUPPORT_5G
      {
        const __FlashStringHelper *wifiModeNames[] = {
          ESPEasy::net::wifi::getWifiBandModeString(WIFI_BAND_MODE_2G_ONLY),
          ESPEasy::net::wifi::getWifiBandModeString(WIFI_BAND_MODE_5G_ONLY),
          ESPEasy::net::wifi::getWifiBandModeString(WIFI_BAND_MODE_AUTO),
        };
        const int wifiModeOptions[]     = { WIFI_BAND_MODE_2G_ONLY, WIFI_BAND_MODE_5G_ONLY, WIFI_BAND_MODE_AUTO };
        constexpr int nrWifiModeOptions = NR_ELEMENTS(wifiModeNames);
        const FormSelectorOptions selector(
          nrWifiModeOptions,
          wifiModeNames,
          wifiModeOptions);
        selector.addFormSelector(
          getLabel(LabelType::WIFI_BAND_MODE),
          getInternalLabel(LabelType::WIFI_BAND_MODE),
          Settings.WiFi_band_mode());
      }
# endif // if CONFIG_SOC_WIFI_SUPPORT_5G


      addFormSubHeader(F("WiFi Power"));
      addFormCheckBox(LabelType::FORCE_WIFI_NOSLEEP, Settings.WifiNoneSleep());
# if FEATURE_SET_WIFI_TX_PWR
      addFormFloatNumberBox(LabelType::WIFI_TX_MAX_PWR, Settings.getWiFi_TX_power(), 0.0f, MAX_TX_PWR_DBM_11b, 2, 0.25f);
      addFormNumericBox(LabelType::WIFI_SENS_MARGIN, Settings.WiFi_sensitivity_margin, -20, 30);
      addFormCheckBox(LabelType::WIFI_SEND_AT_MAX_TX_PWR, Settings.UseMaxTXpowerForSending());
# endif // if FEATURE_SET_WIFI_TX_PWR

      addFormSubHeader(F("WiFi Tweaks"));

      addFormCheckBox(LabelType::RESTART_WIFI_LOST_CONN, Settings.WiFiRestart_connection_lost());

      addFormNumericBox(LabelType::WIFI_NR_EXTRA_SCANS, Settings.NumberExtraWiFiScans, 0, 5);
      addFormCheckBox(LabelType::WIFI_USE_LAST_CONN_FROM_RTC, Settings.UseLastWiFiFromRTC());

# ifndef ESP32
      addFormCheckBox(LabelType::WAIT_WIFI_CONNECT,           Settings.WaitWiFiConnect());
# endif
      addFormCheckBox(LabelType::SDK_WIFI_AUTORECONNECT,      Settings.SDK_WiFi_autoreconnect());

# ifdef SUPPORT_ARP
      addFormCheckBox(LabelType::PERIODICAL_GRAT_ARP,         Settings.gratuitousARP());
# endif // ifdef SUPPORT_ARP

      break;
    }

    case NWPlugin::Function::NWPLUGIN_INIT:
    {
      initNWPluginData(event->NetworkIndex, new (std::nothrow) ESPEasy::net::wifi::NW001_data_struct_WiFi_STA(event->NetworkIndex));
      ESPEasy::net::wifi::NW001_data_struct_WiFi_STA *NW_data =
        static_cast<ESPEasy::net::wifi::NW001_data_struct_WiFi_STA *>(getNWPluginData(event->NetworkIndex));

      if (NW_data) {
        success = NW_data->init(event);
      }
      break;
    }

    case NWPlugin::Function::NWPLUGIN_EXIT:
    {
      ESPEasy::net::wifi::NW001_data_struct_WiFi_STA *NW_data =
        static_cast<ESPEasy::net::wifi::NW001_data_struct_WiFi_STA *>(getNWPluginData(event->NetworkIndex));

      if (NW_data) {
        success = NW_data->exit(event);
      }
      break;
    }

    case NWPlugin::Function::NWPLUGIN_FIFTY_PER_SECOND:
    {
      // FIXME TD-er: Also called in ESPEasy_loop()
      // Is it still needed there?
      ESPEasy::net::wifi::loopWiFi();
      break;
    }
# ifdef ESP32
    case NWPlugin::Function::NWPLUGIN_PRIORITY_ROUTE_CHANGED:
    {
      ESPEasy::net::wifi::NW001_data_struct_WiFi_STA *NW_data =
        static_cast<ESPEasy::net::wifi::NW001_data_struct_WiFi_STA *>(getNWPluginData(event->NetworkIndex));

      if (NW_data) {
        success = NW_data->handle_priority_route_changed();
      }
      break;
    }
# endif // ifdef ESP32

    default:
      break;

  }
  return success;
}

} // namespace net
} // namespace ESPEasy

#endif // ifdef USES_NW001
