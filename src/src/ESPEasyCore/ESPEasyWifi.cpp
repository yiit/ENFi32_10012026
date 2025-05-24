#include "../ESPEasyCore/ESPEasyWifi.h"

#if FEATURE_WIFI

# include "../../ESPEasy-Globals.h"
# include "../DataStructs/TimingStats.h"
# include "../ESPEasyCore/ESPEasyNetwork.h"
# include "../ESPEasyCore/ESPEasyWifi_ProcessEvent.h"
# include "../ESPEasyCore/ESPEasy_Log.h"
# include "../ESPEasyCore/Serial.h"
# include "../Globals/ESPEasyWiFiEvent.h"
# include "../Globals/EventQueue.h"
# include "../Globals/NetworkState.h"
# include "../Globals/Nodes.h"
# include "../Globals/RTC.h"
# include "../Globals/SecuritySettings.h"
# include "../Globals/Services.h"
# include "../Globals/Settings.h"
# include "../Globals/WiFi_AP_Candidates.h"
# include "../Helpers/ESPEasy_time_calc.h"
# include "../Helpers/Hardware_defines.h"
# include "../Helpers/Misc.h"
# include "../Helpers/Networking.h"
# include "../Helpers/StringConverter.h"
# include "../Helpers/StringGenerator_WiFi.h"
# include "../Helpers/StringProvider.h"


# include "../ESPEasyCore/ESPEasyWifi_abstracted.h"


# ifdef ESP32
#  include <WiFiGeneric.h>
#  include <esp_wifi.h> // Needed to call ESP-IDF functions like esp_wifi_....

#  include <esp_phy_init.h>
# endif // ifdef ESP32


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

     //  ESPEasy::net::wifi::setWifiMode(WIFI_OFF);

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

void SetWiFiTXpower(float dBm) { SetWiFiTXpower(dBm, WiFi.RSSI()); }

void SetWiFiTXpower(float dBm, float rssi) {
  const WiFiMode_t cur_mode = WiFi.getMode();

  if (cur_mode == WIFI_OFF) {
    return;
  }

  if (Settings.UseMaxTXpowerForSending()) {
    dBm = 30; // Just some max, will be limited later
  }

  // Range ESP32  : -1dBm - 20dBm
  // Range ESP8266: 0dBm - 20.5dBm
  float maxTXpwr;
  float threshold = GetRSSIthreshold(maxTXpwr);
  #  ifdef ESP8266
  float minTXpwr{};
  #  endif
  #  ifdef ESP32
  float minTXpwr = -1.0f;
  #  endif

  threshold += Settings.WiFi_sensitivity_margin; // Margin in dBm on top of threshold

  // Assume AP sends with max set by ETSI standard.
  // 2.4 GHz: 100 mWatt (20 dBm)
  // US and some other countries allow 1000 mW (30 dBm)
  // We cannot send with over 20 dBm, thus it makes no sense to force higher TX power all the time.
  const float newrssi = rssi - 20;

  if (newrssi < threshold) {
    minTXpwr = threshold - newrssi;
  }

  if (minTXpwr > maxTXpwr) {
    minTXpwr = maxTXpwr;
  }

  if (dBm > maxTXpwr) {
    dBm = maxTXpwr;
  } else if (dBm < minTXpwr) {
    dBm = minTXpwr;
  }

  ESPEasy::net::wifi::SetWiFiTXpower(dBm);

  if (WiFiEventData.wifi_TX_pwr < dBm) {
    // Will increase the TX power, give power supply of the unit some rest
    delay(1);
  }

  WiFiEventData.wifi_TX_pwr = dBm;

  delay(0);
  #  ifndef BUILD_NO_DEBUG

  if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
    const int TX_pwr_int   = WiFiEventData.wifi_TX_pwr * 4;
    const int maxTXpwr_int = maxTXpwr * 4;

    if (TX_pwr_int != maxTXpwr_int) {
      static int last_log = -1;

      if (TX_pwr_int != last_log) {
        last_log = TX_pwr_int;
        String log = strformat(
          F("WiFi : Set TX power to %ddBm sensitivity: %ddBm"),
          static_cast<int>(dBm),
          static_cast<int>(threshold));

        if (rssi < 0) {
          log += strformat(F(" RSSI: %ddBm"), static_cast<int>(rssi));
        }
        addLogMove(LOG_LEVEL_DEBUG, log);
      }
    }
  }
  #  endif // ifndef BUILD_NO_DEBUG
}

# endif // if FEATURE_SET_WIFI_TX_PWR

float GetRSSIthreshold(float& maxTXpwr) {
  maxTXpwr = Settings.getWiFi_TX_power();
  return ESPEasy::net::wifi::GetRSSIthreshold(maxTXpwr);
}

int GetRSSI_quality() {
  long rssi = WiFi.RSSI();

  if (-50 < rssi) { return 10; }

  if (rssi <= -98) { return 0;  }
  rssi = rssi + 97; // Range 0..47 => 1..9
  return (rssi / 5) + 1;
}

WiFiConnectionProtocol getConnectionProtocol() { return ESPEasy::net::wifi::getConnectionProtocol(); }

# ifdef ESP32

int64_t WiFi_get_TSF_time() { return esp_wifi_get_tsf_time(WIFI_IF_STA); }

# endif // ifdef ESP32

// ********************************************************************************
// Disconnect from Wifi AP
// ********************************************************************************
void WifiDisconnect() { ESPEasyWiFi.disconnect(); }

// ********************************************************************************
// Scan WiFi network
// ********************************************************************************
bool WiFiScanAllowed() {
  if (WiFi_AP_Candidates.scanComplete() == WIFI_SCAN_RUNNING) {
    return false;
  }
  return WiFiEventData.processedConnect;
}

void WifiScan(bool async, uint8_t channel) {
  ESPEasy::net::wifi::setSTA(true);

  if (!WiFiScanAllowed()) {
    return;
  }
# ifdef ESP32

  // TD-er: Don't run async scan on ESP32.
  // Since IDF 4.4 it seems like the active channel may be messed up when running async scan
  // Perform a disconnect after scanning.
  // See: https://github.com/letscontrolit/ESPEasy/pull/3579#issuecomment-967021347
  async = false;

  if (Settings.IncludeHiddenSSID()) {
    ESPEasy::net::wifi::setWiFiCountryPolicyManual();
  }


# endif // ifdef ESP32

  START_TIMER;
  WiFiEventData.lastScanMoment.setNow();
  # ifndef BUILD_NO_DEBUG

  if (loglevelActiveFor(LOG_LEVEL_INFO)) {
    if (channel == 0) {
      addLog(LOG_LEVEL_INFO, F("WiFi : Start network scan all channels"));
    } else {
      addLogMove(LOG_LEVEL_INFO, strformat(F("WiFi : Start network scan ch: %d "), channel));
    }
  }
  # endif // ifndef BUILD_NO_DEBUG
  bool show_hidden = true;
  WiFiEventData.processedScanDone = false;
  WiFiEventData.lastGetScanMoment.setNow();
  WiFiEventData.lastScanChannel = channel;

  unsigned int nrScans = 1 + (async ? 0 : Settings.NumberExtraWiFiScans);

  while (nrScans > 0) {
    if (!async) {
      WiFi_AP_Candidates.begin_sync_scan();
      FeedSW_watchdog();
    }
    --nrScans;
# ifdef ESP8266
#  if FEATURE_ESP8266_DIRECT_WIFI_SCAN
    {
      static bool FIRST_SCAN = true;

      struct scan_config config;
      memset(&config, 0, sizeof(config));
      config.ssid        = nullptr;
      config.bssid       = nullptr;
      config.channel     = channel;
      config.show_hidden = show_hidden ? 1 : 0;
      config.scan_type   = WIFI_SCAN_TYPE_ACTIVE;

      if (FIRST_SCAN) {
        config.scan_time.active.min = 100;
        config.scan_time.active.max = 200;
      } else {
        config.scan_time.active.min = 400;
        config.scan_time.active.max = 500;
      }
      FIRST_SCAN = false;
      wifi_station_scan(&config, &onWiFiScanDone);

      if (!async) {
        // will resume when SYSTEM_EVENT_SCAN_DONE event is fired
        do
        {
          delay(0);
        } while (!WiFiEventData.processedScanDone);
      }

    }
#  else // if FEATURE_ESP8266_DIRECT_WIFI_SCAN
    WiFi.scanNetworks(async, show_hidden, channel);
#  endif // if FEATURE_ESP8266_DIRECT_WIFI_SCAN
# endif // ifdef ESP8266
# ifdef ESP32
    const bool passive             = Settings.PassiveWiFiScan();
    const uint32_t max_ms_per_chan = 120;
#ifndef ESP32C5
    // C5 scans both 2.4 and 5 GHz band, which takes much longer
    WiFi.setScanTimeout(14 * max_ms_per_chan * 2);
#endif
    WiFi.scanNetworks(async, show_hidden, passive, max_ms_per_chan /*, channel */);
# endif // ifdef ESP32

    if (!async) {
      FeedSW_watchdog();
      processScanDone();
    }
  }
# if FEATURE_TIMING_STATS

  if (async) {
    STOP_TIMER(WIFI_SCAN_ASYNC);
  } else {
    STOP_TIMER(WIFI_SCAN_SYNC);
  }
# endif // if FEATURE_TIMING_STATS

# ifdef ESP32
#  if ESP_IDF_VERSION_MAJOR < 5
  RTC.clearLastWiFi();

  if (WiFiConnected()) {
    #   ifndef BUILD_NO_DEBUG
    addLog(LOG_LEVEL_INFO, F("WiFi : Disconnect after scan"));
    #   endif

    const bool needReconnect = WiFiEventData.wifiConnectAttemptNeeded;
    WifiDisconnect();
    WiFiEventData.wifiConnectAttemptNeeded = needReconnect;
  }
#  endif // if ESP_IDF_VERSION_MAJOR < 5
# endif // ifdef ESP32

}

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
    ESPEasy::net::wifi::setWifiMode(cur_wifimode);
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
void setAPinternal(bool enable)
{
  if (enable) {
    // create and store unique AP SSID/PW to prevent ESP from starting AP mode with default SSID and No password!
    // setup ssid for AP Mode when needed
    String softAPSSID = NetworkCreateRFCCompliantHostname();
    String pwd        = SecuritySettings.WifiAPKey;
    IPAddress subnet(DEFAULT_AP_SUBNET);

    if (!WiFi.softAPConfig(apIP, apIP, subnet)) {
      addLog(LOG_LEVEL_ERROR, strformat(
               ("WIFI : [AP] softAPConfig failed! IP: %s, GW: %s, SN: %s"),
               apIP.toString().c_str(),
               apIP.toString().c_str(),
               subnet.toString().c_str()
               )
             );
    }

    int channel = 1;

    if (ESPEasy::net::wifi::WifiIsSTA(WiFi.getMode()) && WiFiConnected()) {
      channel = WiFi.channel();
    }

    if (WiFi.softAP(softAPSSID.c_str(), pwd.c_str(), channel)) {
      eventQueue.add(F("WiFi#APmodeEnabled"));

      if (loglevelActiveFor(LOG_LEVEL_INFO)) {
        addLogMove(LOG_LEVEL_INFO, strformat(
                     F("WIFI : AP Mode enabled. SSID: %s IP: %s ch: %d"),
                     softAPSSID.c_str(),
                     formatIP(WiFi.softAPIP()).c_str(),
                     channel));
      }
    } else {
      if (loglevelActiveFor(LOG_LEVEL_ERROR)) {
        addLogMove(LOG_LEVEL_ERROR, strformat(
                     F("WIFI : Error while starting AP Mode with SSID: %s IP: %s"),
                     softAPSSID.c_str(),
                     formatIP(apIP).c_str()));
      }
    }
    # ifdef ESP32

    # else // ifdef ESP32

    if (wifi_softap_dhcps_status() != DHCP_STARTED) {
      if (!wifi_softap_dhcps_start()) {
        addLog(LOG_LEVEL_ERROR, F("WIFI : [AP] wifi_softap_dhcps_start failed!"));
      }
    }
    # endif // ifdef ESP32
    WiFiEventData.timerAPoff.setMillisFromNow(WIFI_AP_OFF_TIMER_DURATION);
  } else {
    # if FEATURE_DNS_SERVER

    if (dnsServerActive) {
      dnsServerActive = false;
      dnsServer.stop();
    }
    # endif // if FEATURE_DNS_SERVER
  }
}

bool WiFiUseStaticIP() { return Settings.IP[0] != 0 && Settings.IP[0] != 255; }

bool wifiAPmodeActivelyUsed()
{
  if (!ESPEasy::net::wifi::WifiIsAP(WiFi.getMode()) || (!WiFiEventData.timerAPoff.isSet())) {
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

#endif // if FEATURE_WIFI
