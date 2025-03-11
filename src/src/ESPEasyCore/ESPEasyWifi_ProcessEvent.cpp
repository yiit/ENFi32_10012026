#include "../ESPEasyCore/ESPEasyWifi_ProcessEvent.h"

#include "../../ESPEasy-Globals.h"

#if FEATURE_ETHERNET
# include "../ESPEasyCore/ESPEasyEth_ProcessEvent.h"
#endif
#include "../ESPEasyCore/ESPEasyNetwork.h"
#include "../ESPEasyCore/ESPEasyWifi.h"
#include "../ESPEasyCore/ESPEasyWifi_abstracted.h"

#include "../Globals/ESPEasyWiFiEvent.h"
#include "../Globals/ESPEasy_Scheduler.h"
#include "../Globals/ESPEasy_time.h"
#include "../Globals/EventQueue.h"
#include "../Globals/MQTT.h"
#include "../Globals/NetworkState.h"
#include "../Globals/RTC.h"
#include "../Globals/SecuritySettings.h"
#include "../Globals/Services.h"
#include "../Globals/Settings.h"
#include "../Globals/WiFi_AP_Candidates.h"

#include "../Helpers/Convert.h"
#include "../Helpers/ESPEasyRTC.h"
#include "../Helpers/ESPEasy_Storage.h"
#include "../Helpers/Network.h"
#include "../Helpers/Networking.h"
#include "../Helpers/PeriodicalActions.h"
#include "../Helpers/StringConverter.h"
#include "../Helpers/StringGenerator_WiFi.h"
#include "../Helpers/StringProvider.h"

// #include "../ESPEasyCore/ESPEasyEth.h"
// #include "../ESPEasyCore/ESPEasyWiFiEvent.h"
// #include "../ESPEasyCore/ESPEasy_Log.h"
// #include "../Helpers/ESPEasy_time_calc.h"
// #include "../Helpers/Misc.h"
// #include "../Helpers/Scheduler.h"

#include "../WebServer/ESPEasy_WebServer.h"

// ********************************************************************************
// Called from the loop() to make sure events are processed as soon as possible.
// These functions are called from Setup() or Loop() and thus may call delay() or yield()
// ********************************************************************************
void handle_unprocessedNetworkEvents()
{
#if FEATURE_ETHERNET
  handle_unprocessedEthEvents();
#endif

  if (active_network_medium == NetworkMedium_t::WIFI) {
    bool processedSomething = false;

    if (!WiFiEventData.processedConnect) {
        #ifndef BUILD_NO_DEBUG
      addLog(LOG_LEVEL_DEBUG, F("WIFI : Entering processConnect()"));
        #endif // ifndef BUILD_NO_DEBUG
      processConnect();
      processedSomething = true;

      // FIXME TD-er: Forcefully set the connected flag for now
      WiFiEventData.setWiFiConnected();
    }

    if (!WiFiEventData.processedGotIP) {
        #ifndef BUILD_NO_DEBUG
      addLog(LOG_LEVEL_DEBUG, F("WIFI : Entering processGotIP()"));
        #endif // ifndef BUILD_NO_DEBUG
      processGotIP();

      processedSomething = true;

      // FIXME TD-er: Forcefully set the GotIP flag for now
      WiFiEventData.setWiFiGotIP();
    }

    if (!WiFiEventData.WiFiServicesInitialized()) {
      WiFiEventData.setWiFiServicesInitialized();
    }

    if (WiFiEventData.WiFiServicesInitialized() && processedSomething) {

      // #ifdef ESP32
      setWebserverRunning(false);
      delay(1);
      setWebserverRunning(true);
      delay(1);

      /*
       #else
              CheckRunningServices();
       #endif
       */
    }
  }

#if FEATURE_ETHERNET
  check_Eth_DNS_valid();
#endif // if FEATURE_ETHERNET

#if FEATURE_ESPEASY_P2P
  updateUDPport(false);
#endif
}

// ********************************************************************************
// Functions to process the data gathered from the events.
// These functions are called from Setup() or Loop() and thus may call delay() or yield()
// ********************************************************************************
void processDisconnect() {}

void processConnect() {
  if (WiFiEventData.processedConnect) { return; }

  /*
       // delay(100); // FIXME TD-er: See https://github.com/letscontrolit/ESPEasy/issues/1987#issuecomment-451644424
       if (checkAndResetWiFi()) {
        return;
       }
   */
  WiFiEventData.processedConnect = true;

  /*
       if (WiFi.status() == WL_DISCONNECTED) {
        // Apparently not really connected
        return;
       }
   */
  WiFiEventData.setWiFiConnected();
  ++WiFiEventData.wifi_reconnects;

  if (WiFi_AP_Candidates.getCurrent().bits.isEmergencyFallback) {
   #ifdef CUSTOM_EMERGENCY_FALLBACK_RESET_CREDENTIALS
    const bool mustResetCredentials = CUSTOM_EMERGENCY_FALLBACK_RESET_CREDENTIALS;
   #else
    const bool mustResetCredentials = false;
   #endif // ifdef CUSTOM_EMERGENCY_FALLBACK_RESET_CREDENTIALS
   #ifdef CUSTOM_EMERGENCY_FALLBACK_START_AP
    const bool mustStartAP = CUSTOM_EMERGENCY_FALLBACK_START_AP;
   #else
    const bool mustStartAP = false;
   #endif // ifdef CUSTOM_EMERGENCY_FALLBACK_START_AP

    if (mustStartAP) {
      int allowedUptimeMinutes = 10;
   #ifdef CUSTOM_EMERGENCY_FALLBACK_ALLOW_MINUTES_UPTIME
      allowedUptimeMinutes = CUSTOM_EMERGENCY_FALLBACK_ALLOW_MINUTES_UPTIME;
   #endif

      if (getUptimeMinutes() < allowedUptimeMinutes) {
        WiFiEventData.timerAPstart.setNow();
      }
    }

    if (mustResetCredentials && !WiFiEventData.performedClearWiFiCredentials) {
      WiFiEventData.performedClearWiFiCredentials = true;
      SecuritySettings.clearWiFiCredentials();
      SaveSecuritySettings();
      WiFiEventData.markDisconnect(WIFI_DISCONNECT_REASON_AUTH_EXPIRE);
      WiFi_AP_Candidates.force_reload();
    }
  }

  if (loglevelActiveFor(LOG_LEVEL_INFO)) {
    const LongTermTimer::Duration connect_duration =
      WiFiEventData.last_wifi_connect_attempt_moment.timeDiff(WiFiEventData.lastConnectMoment);
    String log = strformat(
      F("WIFI : Connected! AP: %s (%s) Ch: %d"),
      WiFi.SSID().c_str(),
      WiFi.BSSIDstr().c_str(),
      RTC.lastWiFiChannel);

    if ((connect_duration > 0ll) && (connect_duration < 30000000ll)) {
      // Just log times when they make sense.
      log += strformat(
        F(" Duration: %d ms"),
        static_cast<int32_t>(connect_duration / 1000));
    }
    addLogMove(LOG_LEVEL_INFO, log);
  }

  //  WiFiEventData.last_wifi_connect_attempt_moment.clear();

  if (Settings.UseRules) {
    if (WiFiEventData.bssid_changed) {
      eventQueue.add(F("WiFi#ChangedAccesspoint"));
    }

    if (WiFiEventData.channel_changed) {
      eventQueue.add(F("WiFi#ChangedWiFichannel"));
    }
  }

  if (useStaticIP()) {
    WiFiEventData.markGotIP(); // in static IP config the got IP event is never fired.
  }
  saveToRTC();

  logConnectionStatus();
}

void processGotIP() {
  if (WiFiEventData.processedGotIP) {
    return;
  }

  if (checkAndResetWiFi()) {
    return;
  }

  IPAddress ip = NetworkLocalIP();

  if (!useStaticIP()) {
   #ifdef ESP8266

    if (!ip.isSet())
   #else // ifdef ESP8266

    if ((ip[0] == 0) && (ip[1] == 0) && (ip[2] == 0) && (ip[3] == 0))
   #endif // ifdef ESP8266
    {
      return;
    }
  }
  const IPAddress gw                          = WiFi.gatewayIP();
  const IPAddress subnet                      = WiFi.subnetMask();
  const LongTermTimer::Duration dhcp_duration = WiFiEventData.lastConnectMoment.timeDiff(WiFiEventData.lastGetIPmoment);
  WiFiEventData.dns0_cache = WiFi.dnsIP(0);
  WiFiEventData.dns1_cache = WiFi.dnsIP(1);

  if (loglevelActiveFor(LOG_LEVEL_INFO)) {
    String log = strformat(
      F("WIFI : %s (%s) GW: %s SN: %s DNS: %s"),
      concat(useStaticIP() ? F("Static IP: ") : F("DHCP IP: "), formatIP(ip)).c_str(),
      NetworkGetHostname().c_str(),
      formatIP(gw).c_str(),
      formatIP(subnet).c_str(),
      getValue(LabelType::DNS).c_str());

    if ((dhcp_duration > 0ll) && (dhcp_duration < 30000000ll)) {
      // Just log times when they make sense.
      log += strformat(F("   duration: %d ms"), static_cast<int32_t>(dhcp_duration / 1000));
    }
    addLogMove(LOG_LEVEL_INFO, log);
  }

  // Might not work in core 2.5.0
  // See https://github.com/esp8266/Arduino/issues/5839
  if ((Settings.IP_Octet != 0) && (Settings.IP_Octet != 255))
  {
    ip[3] = Settings.IP_Octet;

    if (loglevelActiveFor(LOG_LEVEL_INFO)) {
      addLogMove(LOG_LEVEL_INFO, concat(F("IP   : Fixed IP octet:"), formatIP(ip)));
    }
    WiFi.config(ip, gw, subnet, WiFiEventData.dns0_cache, WiFiEventData.dns1_cache);
  }

   #if FEATURE_MQTT
  mqtt_reconnect_count        = 0;
  MQTTclient_should_reconnect = true;
  timermqtt_interval          = 100;
  Scheduler.setIntervalTimer(SchedulerIntervalTimer_e::TIMER_MQTT);
  scheduleNextMQTTdelayQueue();
   #endif // if FEATURE_MQTT
  Scheduler.sendGratuitousARP_now();

  if (Settings.UseRules)
  {
    eventQueue.add(F("WiFi#Connected"));
  }
  statusLED(true);

  //  WiFi.scanDelete();

  if (WiFiEventData.wifiSetup) {
    // Wifi setup was active, Apparently these settings work.
    WiFiEventData.wifiSetup = false;
    SaveSecuritySettings();
  }

  if ((WiFiEventData.WiFiConnected() || WiFi.isConnected()) && hasIPaddr()) {
    WiFiEventData.setWiFiGotIP();
  }
   #if FEATURE_ESPEASY_P2P
  refreshNodeList();
   #endif
  logConnectionStatus();
}

#if FEATURE_USE_IPV6

void processGotIPv6() {
  if (!WiFiEventData.processedGotIP6) {
    WiFiEventData.processedGotIP6 = true;

    if (loglevelActiveFor(LOG_LEVEL_INFO)) {
      addLog(LOG_LEVEL_INFO, String(F("WIFI : STA got IP6 ")) + WiFiEventData.unprocessed_IP6.toString(true));
    }
# if FEATURE_ESPEASY_P2P

    //    updateUDPport(true);
# endif // if FEATURE_ESPEASY_P2P
  }
}

#endif // if FEATURE_USE_IPV6

// A client disconnected from the AP on this node.
void processDisconnectAPmode() {
  if (WiFiEventData.processedDisconnectAPmode) { return; }
  WiFiEventData.processedDisconnectAPmode = true;

#ifndef BUILD_NO_DEBUG

  if (loglevelActiveFor(LOG_LEVEL_INFO)) {
    String log(strformat(
                 F("AP Mode: Client disconnected: %s Connected devices: %u"),
                 WiFiEventData.lastMacDisconnectedAPmode.toString().c_str(),
                 WiFi.softAPgetStationNum()));
    addLogMove(LOG_LEVEL_INFO, log);
  }
#endif // ifndef BUILD_NO_DEBUG
}

// Client connects to AP on this node
void processConnectAPmode() {
  if (WiFiEventData.processedConnectAPmode) { return; }
  WiFiEventData.processedConnectAPmode = true;

  // Extend timer to switch off AP.
  WiFiEventData.timerAPoff.setMillisFromNow(WIFI_AP_OFF_TIMER_DURATION);
#ifndef BUILD_NO_DEBUG

  if (loglevelActiveFor(LOG_LEVEL_INFO)) {
    String log(strformat(
                 F("AP Mode: Client connected: %s Connected devices: %u"),
                 WiFiEventData.lastMacConnectedAPmode.toString().c_str(),
                 WiFi.softAPgetStationNum()));
    addLogMove(LOG_LEVEL_INFO, log);
  }
#endif // ifndef BUILD_NO_DEBUG

  #if FEATURE_DNS_SERVER

  // Start DNS, only used if the ESP has no valid WiFi config
  // It will reply with it's own address on all DNS requests
  // (captive portal concept)
  if (!dnsServerActive) {
    dnsServerActive = true;
    dnsServer.start(DNS_PORT, "*", apIP);
  }
  #endif // if FEATURE_DNS_SERVER
}

// Switch of AP mode when timeout reached and no client connected anymore.
void processDisableAPmode() {
  if (!WiFiEventData.timerAPoff.isSet()) { return; }

  if (!ESPEasy::net::wifi::WifiIsAP(WiFi.getMode())) {
    return;
  }

  // disable AP after timeout and no clients connected.
  if (WiFiEventData.timerAPoff.timeReached() && (WiFi.softAPgetStationNum() == 0)) {
    ESPEasy::net::wifi::setAP(false);
  }

  if (!ESPEasy::net::wifi::WifiIsAP(WiFi.getMode())) {
    WiFiEventData.timerAPoff.clear();

    if (WiFiEventData.wifiConnectAttemptNeeded) {
      // Force a reconnect cycle
      WifiDisconnect();
    }
  }
}

void processScanDone() {
  WiFi_AP_Candidates.load_knownCredentials();

  if (WiFiEventData.processedScanDone) { return; }


  // Better act on the scan done event, as it may get triggered for normal wifi begin calls.
  int8_t scanCompleteStatus = WiFi.scanComplete();

  switch (scanCompleteStatus)
  {
    case 0: // Nothing (yet) found

      if (WiFiEventData.lastGetScanMoment.timeoutReached(5000)) {
        WiFi.scanDelete();
        WiFiEventData.processedScanDone = true;
      }
      return;
    case -1: // WIFI_SCAN_RUNNING

      // FIXME TD-er: Set timeout...
      if (WiFiEventData.lastGetScanMoment.timeoutReached(5000)) {
        #ifndef BUILD_NO_DEBUG
        addLog(LOG_LEVEL_ERROR, F("WiFi : Scan Running Timeout"));
      #endif
        WiFi.scanDelete();
        WiFiEventData.processedScanDone = true;
      }
      return;
    case -2: // WIFI_SCAN_FAILED
      addLog(LOG_LEVEL_ERROR, F("WiFi : Scan failed"));
      WiFi.scanDelete();
      WiFiEventData.processedScanDone = true;
      return;
  }

  WiFiEventData.lastGetScanMoment.setNow();
  WiFiEventData.processedScanDone = true;
#ifndef BUILD_NO_DEBUG

  if (loglevelActiveFor(LOG_LEVEL_INFO)) {
    addLogMove(LOG_LEVEL_INFO, concat(F("WiFi : Scan finished, found: "), scanCompleteStatus));
  }
#endif // ifndef BUILD_NO_DEBUG

#if !FEATURE_ESP8266_DIRECT_WIFI_SCAN
  WiFi_AP_Candidates.process_WiFiscan(scanCompleteStatus);
#endif
  WiFi_AP_Candidates.load_knownCredentials();

  if (WiFi_AP_Candidates.addedKnownCandidate() && !NetworkConnected()) {
    if (!WiFiEventData.wifiConnectInProgress) {
      WiFiEventData.wifiConnectAttemptNeeded = true;
      #ifndef BUILD_NO_DEBUG

      if (WiFi_AP_Candidates.addedKnownCandidate()) {
        addLog(LOG_LEVEL_INFO, F("WiFi : Added known candidate, try to connect"));
      }
      #endif // ifndef BUILD_NO_DEBUG
#ifdef ESP32

      //       ESPEasy::net::wifi::setSTA(false);
#endif // ifdef ESP32
      NetworkConnectRelaxed();
#ifdef USES_ESPEASY_NOW
      temp_disable_EspEasy_now_timer = millis() + 20000;
#endif
    }
  } else if (!WiFiEventData.wifiConnectInProgress && !NetworkConnected()) {
    WiFiEventData.timerAPstart.setNow();
  }

}
