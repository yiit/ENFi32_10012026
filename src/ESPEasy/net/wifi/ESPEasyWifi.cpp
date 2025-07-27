#include "../wifi/ESPEasyWifi.h"

#if FEATURE_WIFI

# include "../../../ESPEasy-Globals.h"
# include "../../../src/DataStructs/TimingStats.h"
# include "../../net/ESPEasyNetwork.h"
# include "../../../src/ESPEasyCore/ESPEasy_Log.h"
# include "../../../src/ESPEasyCore/Serial.h"
# include "../../../src/Globals/EventQueue.h"
# include "../../../src/Globals/Nodes.h"
# include "../../../src/Globals/RTC.h"
# include "../../../src/Globals/SecuritySettings.h"
# include "../../../src/Globals/Services.h"
# include "../../../src/Globals/Settings.h"
# include "../../../src/Globals/WiFi_AP_Candidates.h"
# include "../../../src/Helpers/ESPEasy_time_calc.h"
# include "../../../src/Helpers/Hardware_defines.h"
# include "../../../src/Helpers/Misc.h"
# include "../../../src/Helpers/Networking.h"
# include "../../../src/Helpers/StringConverter.h"
# include "../../../src/Helpers/StringGenerator_WiFi.h"
# include "../../../src/Helpers/StringProvider.h"
# include "../Globals/ESPEasyWiFi.h"
# include "../Globals/ESPEasyWiFiEvent.h"
# include "../Globals/NetworkState.h"
# include "../wifi/ESPEasyWifi_ProcessEvent.h"
# include "../wifi/ESPEasyWifi_abstracted.h"


# ifdef ESP32
#  include <WiFiGeneric.h>
#  include <esp_wifi.h> // Needed to call ESP-IDF functions like esp_wifi_....

#  ifndef ESP32P4
#   include <esp_phy_init.h>
#  endif

# endif // ifdef ESP32


namespace ESPEasy {
namespace net {
namespace wifi {

// ********************************************************************************
// WiFi state
// ********************************************************************************

/*
   WiFi STA states:
   1 STA off                 => ESPEASY_WIFI_DISCONNECTED
   2 STA connecting
   3 STA connected           => ESPEASY_WIFI_CONNECTED
   4 STA got IP              => ESPEASY_WIFI_GOT_IP
   5 STA connected && got IP => ESPEASY_WIFI_SERVICES_INITIALIZED

   N.B. the states are flags, meaning both "connected" and "got IP" must be set
        to be considered ESPEASY_WIFI_SERVICES_INITIALIZED

   The flag wifiConnectAttemptNeeded indicates whether a new connect attempt is needed.
   This is set to true when:
   - Security settings have been saved with AP mode enabled. FIXME TD-er, this may not be the best check.
   - WiFi connect timeout reached  &  No client is connected to the AP mode of the node.
   - Wifi is reset
   - WiFi setup page has been loaded with SSID/pass values.


   WiFi AP mode states:
   1 AP on                        => reset AP disable timer
   2 AP client connect/disconnect => reset AP disable timer
   3 AP off                       => AP disable timer = 0;

   AP mode will be disabled when both apply:
   - AP disable timer (timerAPoff) expired
   - No client is connected to the AP.

   AP mode will be enabled when at least one applies:
   - No valid WiFi settings
   - Start AP timer (timerAPstart) expired

   Start AP timer is set or cleared at:
   - Set timerAPstart when "valid WiFi connection" state is observed.
   - Disable timerAPstart when ESPEASY_WIFI_SERVICES_INITIALIZED wifi state is reached.

   For the first attempt to connect after a cold boot (RTC values are 0), a WiFi scan will be
   performed to find the strongest known SSID.
   This will set RTC.lastBSSID and RTC.lastWiFiChannel

   Quick reconnect (using BSSID/channel of last connection) when both apply:
   - If wifi_connect_attempt < 3
   - RTC.lastBSSID is known
   - RTC.lastWiFiChannel != 0

   Change of wifi settings when both apply:
   - "other" settings valid
   - (wifi_connect_attempt % 2) == 0

   Reset of wifi_connect_attempt to 0 when both apply:
   - connection successful
   - Connection stable (connected for > 5 minutes)

 */

// ********************************************************************************
// Check WiFi connected status
// This is basically the state machine to switch between states:
// - Initiate WiFi reconnect
// - Start/stop of AP mode
// ********************************************************************************
bool WiFiConnected() {
  static uint32_t lastCheckedTime = 0;

  const int32_t timePassed = timePassedSince(lastCheckedTime);

  if (lastCheckedTime != 0) {
    if (timePassed < 10) {
      // Rate limit time spent in WiFiConnected() to max. 100x per sec to process the rest of this function
      return ESPEasyWiFi.connected();
    }
    lastCheckedTime = millis();
  }

  ESPEasyWiFi.loop();
  return ESPEasyWiFi.connected();
}

void WiFiConnectRelaxed() { AttemptWiFiConnect(); }

void AttemptWiFiConnect() {
  if (!WiFiConnected()) {
    ESPEasyWiFi.setup();
  }
  logConnectionStatus();
}

// ********************************************************************************
// Set Wifi config
// ********************************************************************************
bool prepareWiFi() {
  //  ESPEasyWiFi.setup();

  return true;
}

bool checkAndResetWiFi() { return true; }

void resetWiFi() {
  /*   //if (wifiAPmodeActivelyUsed()) return;
     if (WiFiEventData.lastWiFiResetMoment.isSet() && !WiFiEventData.lastWiFiResetMoment.timeoutReached(1000)) {
      // Don't reset WiFi too often
      return;
     }
     FeedSW_watchdog();
     WiFiEventData.clearAll();
     WifiDisconnect();

     // Send this log only after WifiDisconnect() or else sending to syslog may cause issues
     addLog(LOG_LEVEL_INFO, F("Reset WiFi."));

     //  setWifiMode(WIFI_OFF);

     initWiFi();
   */
}

void initWiFi() { ESPEasyWiFi.setup(); }

void loopWiFi() { ESPEasyWiFi.loop(); }

// ********************************************************************************
// Configure WiFi TX power
// ********************************************************************************
# if FEATURE_SET_WIFI_TX_PWR

void SetWiFiTXpower() {
  SetWiFiTXpower(0); // Just some minimal value, will be adjusted in SetWiFiTXpower
}

void SetWiFiTXpower(float dBm)             { doSetWiFiTXpower(dBm, WiFi.RSSI()); }

void SetWiFiTXpower(float dBm, float rssi) { doSetWiFiTXpower(dBm, rssi); }

# endif // if FEATURE_SET_WIFI_TX_PWR

float GetRSSIthreshold(float& maxTXpwr) {
  maxTXpwr = Settings.getWiFi_TX_power();
  return doGetRSSIthreshold(maxTXpwr);
}

int GetRSSI_quality() {
  long rssi = WiFi.RSSI();

  if (-50 < rssi) { return 10; }

  if (rssi <= -98) { return 0;  }
  rssi = rssi + 97; // Range 0..47 => 1..9
  return (rssi / 5) + 1;
}

WiFiConnectionProtocol getConnectionProtocol() { return doGetConnectionProtocol(); }

# ifdef ESP32

int64_t WiFi_get_TSF_time() {
  #  ifndef SOC_WIFI_SUPPORTED
  return 0;
  #  else
  return esp_wifi_get_tsf_time(WIFI_IF_STA);
  #  endif // ifndef SOC_WIFI_SUPPORTED
}

# endif // ifdef ESP32

// ********************************************************************************
// Disconnect from Wifi AP
// ********************************************************************************
void WifiDisconnect() { ESPEasyWiFi.disconnect(); }

// ********************************************************************************
// Scan WiFi network
// ********************************************************************************
bool WiFiScanAllowed() { return doWiFiScanAllowed(); }

// ********************************************************************************
// Scan all Wifi Access Points
// ********************************************************************************
void WiFiScan_log_to_serial()
{
  // Direct Serial is allowed here, since this function will only be called from serial input.
  serialPrintln(F("WIFI : SSID Scan start"));

  if (WiFi_AP_Candidates.scanComplete() <= 0) {
    WiFiMode_t cur_wifimode = WiFi.getMode();
    WifiScan(false);
    setWifiMode(cur_wifimode);
  }

  const int8_t scanCompleteStatus = WiFi_AP_Candidates.scanComplete();

  if (scanCompleteStatus <= 0) {
    serialPrintln(F("WIFI : No networks found"));
  }
  else
  {
    serialPrint(F("WIFI : "));
    serialPrint(String(scanCompleteStatus));
    serialPrintln(F(" networks found"));

    int i = 0;

    for (auto it = WiFi_AP_Candidates.scanned_begin(); it != WiFi_AP_Candidates.scanned_end(); ++it)
    {
      ++i;

      // Print SSID and RSSI for each network found
      serialPrint(F("WIFI : "));
      serialPrint(String(i));
      serialPrint(": ");
      serialPrintln(it->toString());
      delay(10);
    }
  }
  serialPrintln("");
}

// Only internal scope
void setAPinternal(bool enable) { doSetAPinternal(enable); }

bool WiFiUseStaticIP()          { return Settings.IP[0] != 0 && Settings.IP[0] != 255; }

bool wifiAPmodeActivelyUsed()
{
  if (!WifiIsAP(WiFi.getMode()) || (!WiFiEventData.timerAPoff.isSet())) {
    // AP not active or soon to be disabled in processDisableAPmode()
    return false;
  }
  return WiFi.softAPgetStationNum() != 0;

  // FIXME TD-er: is effectively checking for AP active enough or must really check for connected clients to prevent automatic wifi
  // reconnect?
}

void setupStaticIPconfig() {
  setUseStaticIP(WiFiUseStaticIP());

  if (!WiFiUseStaticIP()) { return; }
  const IPAddress ip(Settings.IP);
  const IPAddress gw(Settings.Gateway);
  const IPAddress subnet(Settings.Subnet);
  const IPAddress dns(Settings.DNS);

  WiFiEventData.dns0_cache = dns;

  WiFi.config(ip, gw, subnet, dns);

  if (loglevelActiveFor(LOG_LEVEL_INFO)) {
    addLogMove(LOG_LEVEL_INFO, strformat(
                 F("IP   : Static IP : %s GW: %s SN: %s DNS: %s"),
                 formatIP(ip).c_str(),
                 formatIP(gw).c_str(),
                 formatIP(subnet).c_str(),
                 getValue(LabelType::DNS).c_str()));
  }
}

// ********************************************************************************
// Formatting WiFi related strings
// ********************************************************************************
String formatScanResult(int i, const String& separator) {
  int32_t rssi = 0;

  return formatScanResult(i, separator, rssi);
}

String formatScanResult(int i, const String& separator, int32_t& rssi) {
  WiFi_AP_Candidate tmp(i);

  rssi = tmp.rssi;
  return tmp.toString(separator);
}

void logConnectionStatus() {
  static unsigned long lastLog = 0;

  if ((lastLog != 0) && (timePassedSince(lastLog) < 1000)) {
    return;
  }
  lastLog = millis();
# ifndef BUILD_NO_DEBUG
  #  ifdef ESP8266
  const uint8_t arduino_corelib_wifistatus = WiFi.status();
  const uint8_t sdk_wifistatus             = wifi_station_get_connect_status();

  if ((arduino_corelib_wifistatus == WL_CONNECTED) != (sdk_wifistatus == STATION_GOT_IP)) {
    if (loglevelActiveFor(LOG_LEVEL_ERROR)) {
      String log = F("WiFi : SDK station status differs from Arduino status. SDK-status: ");
      log += SDKwifiStatusToString(sdk_wifistatus);
      log += F(" Arduino status: ");
      log += ArduinoWifiStatusToString(arduino_corelib_wifistatus);
      addLogMove(LOG_LEVEL_ERROR, log);
    }
  }
  #  endif // ifdef ESP8266

  if (loglevelActiveFor(LOG_LEVEL_INFO)) {
    addLogMove(LOG_LEVEL_INFO, strformat(
                 F("WIFI : Arduino wifi status: %s ESPeasy internal wifi status: %s"),
                 ArduinoWifiStatusToString(WiFi.status()).c_str(),
                 WiFiEventData.ESPeasyWifiStatusToString().c_str()));
  }

  /*
     if (loglevelActiveFor(LOG_LEVEL_INFO)) {
      String log;

      switch (WiFi.status()) {
        case WL_NO_SSID_AVAIL: {
          log = F("WIFI : No SSID found matching: ");
          break;
        }
        case WL_CONNECT_FAILED: {
          log = F("WIFI : Connection failed to: ");
          break;
        }
        case WL_DISCONNECTED: {
          log = F("WIFI : WiFi.status() = WL_DISCONNECTED  SSID: ");
          break;
        }
        case WL_IDLE_STATUS: {
          log = F("WIFI : Connection in IDLE state: ");
          break;
        }
        case WL_CONNECTED: {
          break;
        }
        default:
          break;
      }

      if (log.length() > 0) {
        const char *ssid = getLastWiFiSettingsSSID();
        log += ssid;
        addLog(LOG_LEVEL_INFO, log);
      }
     }
   */
# endif // ifndef BUILD_NO_DEBUG
}

bool                       WifiIsAP(WiFiMode_t wifimode)          { return doWifiIsAP(wifimode); }

bool                       WifiIsSTA(WiFiMode_t wifimode)         { return doWifiIsSTA(wifimode); }

const __FlashStringHelper* getWifiModeString(WiFiMode_t wifimode) { return doGetWifiModeString(wifimode); }

# if CONFIG_SOC_WIFI_SUPPORT_5G

const __FlashStringHelper* getWifiBandModeString(wifi_band_mode_t wifiBandMode) { return doGetWifiBandModeString(wifiBandMode); }

# endif // if CONFIG_SOC_WIFI_SUPPORT_5G

bool setSTA(bool enable) { return doSetSTA(enable); }

bool setAP(bool enable)  { return doSetAP(enable); }

bool setSTA_AP(bool sta_enable,
               bool ap_enable) { return doSetSTA_AP(sta_enable, ap_enable); }

bool setWifiMode(WiFiMode_t new_mode) { return doSetWifiMode(new_mode); }

void WifiScan(bool    async,
              uint8_t channel) { doWifiScan(async, channel); }


} // namespace wifi
} // namespace net
} // namespace ESPEasy

#endif // if FEATURE_WIFI
