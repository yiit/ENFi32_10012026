#include "../ESPEasyCore/ESPEasyWifi_abstracted.h"

#if FEATURE_WIFI
# ifdef ESP8266

#  include "../Helpers/StringConverter.h"

#  include "../Globals/ESPEasyWiFiEvent.h"
#  include "../Globals/Settings.h"
#  include "../Globals/WiFi_AP_Candidates.h"

#  include "../ESPEasyCore/ESPEasyWiFiEvent_ESP8266.h"

namespace ESPEasy {
namespace net {
namespace wifi {

bool WiFi_pre_setup()     {
  registerWiFiEventHandler();

   setSTA_AP(false, false);
  delay(100);
  return true;
}

bool WiFi_pre_STA_setup() {
  if (! setSTA(true)) { return false; }

  // Assign to 2 separate bools to make sure both are executed.
  const bool autoConnect   = WiFi.setAutoConnect(false);
  const bool autoReconnect = WiFi.setAutoReconnect(false);

  if (!autoConnect || !autoReconnect) {
    addLog(LOG_LEVEL_ERROR, F("WiFi  : Disabling auto (re)connect failed"));
  }
  delay(100);
  return true;
}

void WiFiDisconnect() {
  // Only call disconnect when STA is active
  if ( WifiIsSTA(WiFi.getMode())) {
    wifi_station_disconnect();
  }
  station_config conf{};
  memset(&conf, 0, sizeof(conf));
  ETS_UART_INTR_DISABLE();
  wifi_station_set_config_current(&conf);
  ETS_UART_INTR_ENABLE();
}

bool  WifiIsAP(WiFiMode_t wifimode)  { return (wifimode == WIFI_AP) || (wifimode == WIFI_AP_STA); }

bool  WifiIsSTA(WiFiMode_t wifimode) { return (wifimode & WIFI_STA) != 0; }

bool setWifiMode(WiFiMode_t new_mode)
{
  const WiFiMode_t cur_mode = WiFi.getMode();

  // Made this static flag an int as ESP8266 and ESP32 differ in the "not set" values
  static int8_t processing_wifi_mode = -1;

  if (cur_mode == new_mode) {
    return true;
  }

  if (processing_wifi_mode == static_cast<int8_t>(new_mode)) {
    // Prevent loops
    return true;
  }
  processing_wifi_mode = static_cast<int8_t>(new_mode);


   if (! WifiIsSTA(new_mode)) {
        // calls lwIP's dhcp_stop(),
        // safe to call even if not started
        // See: https://github.com/esp8266/Arduino/pull/5703/files
        wifi_station_dhcpc_stop();
   }

  if (new_mode != WIFI_OFF) {
    // See: https://github.com/esp8266/Arduino/issues/6172#issuecomment-500457407
    WiFi.forceSleepWake(); // Make sure WiFi is really active.
    delay(100);
  }

  addLog(LOG_LEVEL_INFO, concat(F("WIFI : Set WiFi to "), getWifiModeString(new_mode)));

  int retry = 2;

  while (!WiFi.mode(new_mode) && retry > 0) {
    delay(100);
    --retry;
  }
  retry = 2;

  while (WiFi.getMode() != new_mode && retry > 0) {
    addLog(LOG_LEVEL_INFO, F("WIFI : mode not yet set"));
    delay(100);
    --retry;
  }

  if (WiFi.getMode() != new_mode) {
    addLog(LOG_LEVEL_ERROR, F("WIFI : Cannot set mode!!!!!"));
    return false;
  }


  if (new_mode == WIFI_OFF) {
    WiFi.forceSleepBegin();
    delay(1);
  }


#  if FEATURE_MDNS
  #   ifdef ESP8266

  // notifyAPChange() is not present in the ESP32 MDNSResponder
  MDNS.notifyAPChange();
  #   endif // ifdef ESP8266
  #  endif // if FEATURE_MDNS
  return true;
}

void removeWiFiEventHandler() {
  // Not needed for ESP8266
}

void registerWiFiEventHandler()
{
  // WiFi event handlers
  static bool handlers_initialized = false;

  if (!handlers_initialized) {
    stationConnectedHandler          = WiFi.onStationModeConnected(onConnected);
    stationDisconnectedHandler       = WiFi.onStationModeDisconnected(onDisconnect);
    stationGotIpHandler              = WiFi.onStationModeGotIP(onGotIP);
    stationModeDHCPTimeoutHandler    = WiFi.onStationModeDHCPTimeout(onDHCPTimeout);
    stationModeAuthModeChangeHandler = WiFi.onStationModeAuthModeChanged(onStationModeAuthModeChanged);
    APModeStationConnectedHandler    = WiFi.onSoftAPModeStationConnected(onConnectedAPmode);
    APModeStationDisconnectedHandler = WiFi.onSoftAPModeStationDisconnected(onDisconnectedAPmode);
    handlers_initialized             = true;
  }
}

float GetRSSIthreshold(float& maxTXpwr) {
  maxTXpwr = Settings.getWiFi_TX_power();

  float threshold = WIFI_SENSITIVITY_n;

  switch (getConnectionProtocol())
  {
    case WiFiConnectionProtocol::WiFi_Protocol_11b:
      threshold = WIFI_SENSITIVITY_11b;

      if (maxTXpwr > MAX_TX_PWR_DBM_11b) { maxTXpwr = MAX_TX_PWR_DBM_11b; }
      break;
    case WiFiConnectionProtocol::WiFi_Protocol_11g:
      threshold = WIFI_SENSITIVITY_54g;

      if (maxTXpwr > MAX_TX_PWR_DBM_54g) { maxTXpwr = MAX_TX_PWR_DBM_54g; }
      break;
    case WiFiConnectionProtocol::WiFi_Protocol_11n:
      threshold = WIFI_SENSITIVITY_n;

      if (maxTXpwr > MAX_TX_PWR_DBM_n) { maxTXpwr = MAX_TX_PWR_DBM_n; }
      break;
    case WiFiConnectionProtocol::Unknown:
      break;
  }
  return threshold;
}

WiFiConnectionProtocol getConnectionProtocol()
{
  if (WiFi.RSSI() < 0) {
    switch (wifi_get_phy_mode())
    {
      case PHY_MODE_11B:
        return WiFiConnectionProtocol::WiFi_Protocol_11b;
      case PHY_MODE_11G:
        return WiFiConnectionProtocol::WiFi_Protocol_11g;
      case PHY_MODE_11N:
        return WiFiConnectionProtocol::WiFi_Protocol_11n;
    }
  }
  return WiFiConnectionProtocol::Unknown;
}

#  if FEATURE_SET_WIFI_TX_PWR

void doSetWiFiTXpower(float& dBm) { WiFi.setOutputPower(dBm); }

#  endif // if FEATURE_SET_WIFI_TX_PWR

void setConnectionSpeed(bool ForceWiFi_bg_mode) {
  // ESP8266 only supports 802.11g mode when running in STA+AP
  const bool forcedByAPmode =  WifiIsAP(WiFi.getMode());
  WiFiPhyMode_t phyMode     = (ForceWiFi_bg_mode || forcedByAPmode) ? WIFI_PHY_MODE_11G : WIFI_PHY_MODE_11N;

  if (!forcedByAPmode) {
    const WiFi_AP_Candidate candidate = WiFi_AP_Candidates.getCurrent();

    if (candidate.phy_known() && (candidate.bits.phy_11g != candidate.bits.phy_11n)) {
      if ((WIFI_PHY_MODE_11G == phyMode) && !candidate.bits.phy_11g) {
        phyMode = WIFI_PHY_MODE_11N;
        addLog(LOG_LEVEL_INFO, F("WIFI : AP is set to 802.11n only"));
      } else if ((WIFI_PHY_MODE_11N == phyMode) && !candidate.bits.phy_11n) {
        phyMode = WIFI_PHY_MODE_11G;
        addLog(LOG_LEVEL_INFO, F("WIFI : AP is set to 802.11g only"));
      }
    } else {
      bool useAlternate = WiFiEventData.connectionFailures > 10;

      if (useAlternate) {
        phyMode = (WIFI_PHY_MODE_11G == phyMode) ? WIFI_PHY_MODE_11N : WIFI_PHY_MODE_11G;
      }
    }
  } else {
    // No need to perform a next attempt.
    WiFi_AP_Candidates.markAttempt();
  }

  if (WiFi.getPhyMode() == phyMode) {
    return;
  }
  #  ifndef BUILD_NO_DEBUG

  if (loglevelActiveFor(LOG_LEVEL_INFO)) {
    String log = concat(F("WIFI : Set to 802.11"), (WIFI_PHY_MODE_11G == phyMode) ? 'g' : 'n');

    if (forcedByAPmode) {
      log += (F(" (AP+STA mode)"));
    }

    if (Settings.ForceWiFi_bg_mode()) {
      log += F(" Force B/G mode");
    }
    addLogMove(LOG_LEVEL_INFO, log);
  }
  #  endif // ifndef BUILD_NO_DEBUG
  WiFi.setPhyMode(phyMode);
}

void setWiFiNoneSleep() { WiFi.setSleepMode(WIFI_NONE_SLEEP); }

void setWiFiEcoPowerMode()
{
  // Allow light sleep during idle times
  WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
}

void setWiFiDefaultPowerMode() { WiFi.setSleepMode(WIFI_MODEM_SLEEP); }

void setWiFiCountryPolicyManual() {
  // Not yet implemented/required for ESP8266
}

} // namespace wifi
} // namespace net
} // namespace ESPEasy

# endif // ifdef ESP8266
#endif // if FEATURE_WIFI
