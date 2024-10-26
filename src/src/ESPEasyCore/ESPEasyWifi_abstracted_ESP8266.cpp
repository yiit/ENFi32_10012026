#include "../ESPEasyCore/ESPEasyWifi_abstrated.h"

#ifdef ESP8266

# include "../Globals/ESPEasyWiFiEvent.h"
# include "../Globals/WiFi_AP_Candidates.h"

# include "../ESPEasyCore/ESPEasyWiFiEvent_ESP8266.h"

namespace ESPEasy_WiFi_abstraction {
bool WiFiConnected()
{
  bool wifi_isconnected = WiFi.isConnected();

  // Perform check on SDK function, see: https://github.com/esp8266/Arduino/issues/7432
  station_status_t status = wifi_station_get_connect_status();

  switch (status) {
    case STATION_GOT_IP:
      wifi_isconnected = true;
      break;
    case STATION_NO_AP_FOUND:
    case STATION_CONNECT_FAIL:
    case STATION_WRONG_PASSWORD:
      wifi_isconnected = false;
      break;
    case STATION_IDLE:
    case STATION_CONNECTING:
      break;

    default:
      wifi_isconnected = false;
      break;
  }
  return wifi_isconnected;
}

void WiFiDisconnect() {
  // Only call disconnect when STA is active
  if (ESPEasy_WiFi_abstraction::WifiIsSTA(WiFi.getMode())) {
    wifi_station_disconnect();
  }
  station_config conf{};
  memset(&conf, 0, sizeof(conf));
  ETS_UART_INTR_DISABLE();
  wifi_station_set_config_current(&conf);
  ETS_UART_INTR_ENABLE();
}

bool WifiIsAP(WiFiMode_t wifimode)
{
  return (wifimode == WIFI_AP) || (wifimode == WIFI_AP_STA);
}

bool WifiIsSTA(WiFiMode_t wifimode)
{
  return (wifimode & WIFI_STA) != 0;
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
  float threshold = WIFI_SENSITIVITY_n;

  switch (getConnectionProtocol()) {
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
    switch (wifi_get_phy_mode()) {
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

# if FEATURE_SET_WIFI_TX_PWR
void SetWiFiTXpower(float& dBm)
{
  WiFi.setOutputPower(dBm);
}

# endif // if FEATURE_SET_WIFI_TX_PWR

void setConnectionSpeed(bool ForceWiFi_bg_mode) {
  // ESP8266 only supports 802.11g mode when running in STA+AP
  const bool forcedByAPmode = ESPEasy_WiFi_abstraction::WifiIsAP(WiFi.getMode());
  WiFiPhyMode_t phyMode = (ForceWiFi_bg_mode || forcedByAPmode) ? WIFI_PHY_MODE_11G : WIFI_PHY_MODE_11N;
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
  #ifndef BUILD_NO_DEBUG
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
  #endif
  WiFi.setPhyMode(phyMode);
}

void setWiFiNoneSleep()
{
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
}

void setWiFiEcoPowerMode()
{
  // Allow light sleep during idle times
  WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
}

void setWiFiDefaultPowerMode()
{
  WiFi.setSleepMode(WIFI_MODEM_SLEEP);
}






} // namespace ESPEasy_WiFi_abstraction


#endif // ifdef ESP8266
