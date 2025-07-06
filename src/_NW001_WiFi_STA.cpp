#include "ESPEasy_common.h"

#ifdef USES_NW001

// #######################################################################################################
// ########################### Network Plugin 001: WiFi Station ##########################################
// #######################################################################################################

# define NWPLUGIN_001
# define NWPLUGIN_ID_001         1
# define NWPLUGIN_NAME_001       "WiFi Station"

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

bool NWPlugin_001(NWPlugin::Function function, struct EventStruct *event, String& string)
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
      string = F(NWPLUGIN_NAME_001);
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
      Settings.HiddenSSID_SlowConnectPerBSSID(isFormItemChecked(LabelType::HIDDEN_SSID_SLOW_CONNECT));
      Settings.SDK_WiFi_autoreconnect(isFormItemChecked(LabelType::SDK_WIFI_AUTORECONNECT));
# ifdef ESP32
      Settings.PassiveWiFiScan(isFormItemChecked(LabelType::WIFI_PASSIVE_SCAN));
# endif
# if CONFIG_SOC_WIFI_SUPPORT_5G
      Settings.WiFi_band_mode(static_cast<wifi_band_mode_t>(getFormItemInt(getInternalLabel(LabelType::WIFI_BAND_MODE))));
# endif
# if FEATURE_USE_IPV6
      Settings.EnableIPv6(isFormItemChecked(LabelType::ENABLE_IPV6));
# endif


      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_LOAD:
    {
      addFormSubHeader(F("Wifi Settings"));

      addFormTextBox(getLabel(LabelType::SSID), F("ssid"), SecuritySettings.WifiSSID, 31);
      addFormPasswordBox(F("WPA Key"), F("key"), SecuritySettings.WifiKey, 63);
      addFormTextBox(F("Fallback SSID"), F("ssid2"), SecuritySettings.WifiSSID2, 31);
      addFormPasswordBox(F("Fallback WPA Key"), F("key2"), SecuritySettings.WifiKey2, 63);
      addFormNote(F("WPA Key must be at least 8 characters long"));

      addFormCheckBox(LabelType::CONNECT_HIDDEN_SSID,      Settings.IncludeHiddenSSID());

# ifdef ESP32
      addFormCheckBox(LabelType::WIFI_PASSIVE_SCAN,        Settings.PassiveWiFiScan());
# endif

      addFormCheckBox(LabelType::HIDDEN_SSID_SLOW_CONNECT, Settings.HiddenSSID_SlowConnectPerBSSID());

      addFormSubHeader(F("WiFi IP Settings"));

      addFormIPBox(F("ESP WiFi IP"),         F("espip"),      Settings.IP);
      addFormIPBox(F("ESP WiFi Gateway"),    F("espgateway"), Settings.Gateway);
      addFormIPBox(F("ESP WiFi Subnetmask"), F("espsubnet"),  Settings.Subnet);
      addFormIPBox(F("ESP WiFi DNS"),        F("espdns"),     Settings.DNS);
      addFormNote(F("Leave empty for DHCP"));

      addFormSubHeader(F("WiFi Tweaks"));

      addFormCheckBox(LabelType::FORCE_WIFI_BG,          Settings.ForceWiFi_bg_mode());

      addFormCheckBox(LabelType::RESTART_WIFI_LOST_CONN, Settings.WiFiRestart_connection_lost());
      addFormCheckBox(LabelType::FORCE_WIFI_NOSLEEP,     Settings.WifiNoneSleep());

# ifdef SUPPORT_ARP
      addFormCheckBox(LabelType::PERIODICAL_GRAT_ARP,    Settings.gratuitousARP());
# endif // ifdef SUPPORT_ARP

# if FEATURE_SET_WIFI_TX_PWR
      addFormFloatNumberBox(LabelType::WIFI_TX_MAX_PWR, Settings.getWiFi_TX_power(), 0.0f, MAX_TX_PWR_DBM_11b, 2, 0.25f);
      addFormNumericBox(LabelType::WIFI_SENS_MARGIN, Settings.WiFi_sensitivity_margin, -20, 30);
      addFormCheckBox(LabelType::WIFI_SEND_AT_MAX_TX_PWR, Settings.UseMaxTXpowerForSending());
# endif // if FEATURE_SET_WIFI_TX_PWR
      addFormNumericBox(LabelType::WIFI_NR_EXTRA_SCANS, Settings.NumberExtraWiFiScans, 0, 5);
      addFormCheckBox(LabelType::WIFI_USE_LAST_CONN_FROM_RTC, Settings.UseLastWiFiFromRTC());

# ifndef ESP32
      addFormCheckBox(LabelType::WAIT_WIFI_CONNECT,           Settings.WaitWiFiConnect());
# endif
      addFormCheckBox(LabelType::SDK_WIFI_AUTORECONNECT,      Settings.SDK_WiFi_autoreconnect());
      addFormCheckBox(LabelType::HIDDEN_SSID_SLOW_CONNECT,    Settings.HiddenSSID_SlowConnectPerBSSID());
# ifdef ESP32
      addFormCheckBox(LabelType::WIFI_PASSIVE_SCAN,           Settings.PassiveWiFiScan());
# endif
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

# if FEATURE_USE_IPV6
      addFormCheckBox(LabelType::ENABLE_IPV6, Settings.EnableIPv6());
# endif


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

#endif // ifdef USES_NW001
