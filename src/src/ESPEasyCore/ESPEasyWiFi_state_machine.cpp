#include "../ESPEasyCore/ESPEasyWiFi_state_machine.h"

#if FEATURE_WIFI

# include "../ESPEasyCore/ESPEasyWifi.h"
# include "../ESPEasyCore/ESPEasyWifi_abstracted.h"


# include "../ESPEasyCore/ESPEasyNetwork.h" // for setNetworkMedium, however this should not be part of the WiFi code

# include "../ESPEasyCore/ESPEasy_Log.h"

# include "../Globals/ESPEasyWiFiEvent.h"
# include "../Globals/RTC.h"
# include "../Globals/Settings.h"
# include "../Globals/WiFi_AP_Candidates.h"

# include "../Helpers/StringConverter.h"
# include "../Helpers/StringGenerator_WiFi.h"

namespace ESPEasy {
namespace net {
namespace wifi {

    # define WIFI_STATE_MACHINE_STA_CONNECTING_TIMEOUT   10000
    # define WIFI_STATE_MACHINE_STA_AP_SCANNING_TIMEOUT  10000
    # define WIFI_STATE_MACHINE_STA_SCANNING_TIMEOUT     10000

void ESPEasyWiFi_t::setup() {
  // TODO TD-er: Must maybe also call 'disable()' first?

  // TODO TD-er: Load settings

  // TODO TD-er: Check if settings have changed.

  if (_disabledAtBoot) {
    disable();
    return;
  }

  WiFi_pre_setup();
  begin();
}

void ESPEasyWiFi_t::enable()  {}

void ESPEasyWiFi_t::disable() {}

void ESPEasyWiFi_t::begin()   { setState(WiFiState_e::IdleWaiting, 100); }

void ESPEasyWiFi_t::loop()
{
  // TODO TD-er: Must inspect WiFiEventData to see if we need to update some state here.


  if (_state != WiFiState_e::IdleWaiting) {
    if (_callbackError ||
        (_state_timeout.isSet() && _state_timeout.timeReached()))
    {
      // TODO TD-er: Must check what error was given???
      _callbackError = false;
      setState(WiFiState_e::WiFiOFF);
    }
  }

  switch (_state)
  {
    case WiFiState_e::Disabled:
      // Do nothing here, as the device is disabled.
      break;
    case WiFiState_e::WiFiOFF:
      begin();

      //      setState(WiFiState_e::IdleWaiting, 100);
      break;
    case WiFiState_e::AP_only:
      break;
    case WiFiState_e::IdleWaiting:

      if (_state_timeout.timeReached()) {
        // Do we have candidate to connect to ?
        if (WiFi_AP_Candidates.hasCandidates()) {
          setState(WiFiState_e::STA_Connecting, WIFI_STATE_MACHINE_STA_CONNECTING_TIMEOUT);
        } else if (WifiIsAP(WiFi.getMode())) {
          // TODO TD-er: Must check if any client is connected.
          // If not, then we can disable AP mode and switch to WiFiState_e::STA_Scanning
          setState(WiFiState_e::STA_AP_Scanning, WIFI_STATE_MACHINE_STA_AP_SCANNING_TIMEOUT);
        } else {
          setState(WiFiState_e::STA_Scanning, WIFI_STATE_MACHINE_STA_SCANNING_TIMEOUT);
        }
      }
      break;
    case WiFiState_e::STA_Scanning:
    case WiFiState_e::STA_AP_Scanning:

      if (_state_timeout.timeReached() || (WiFi.scanComplete() >= 0)) {
        setState(WiFiState_e::WiFiOFF, 100);
      }

      // Check if scanning is finished
      // When scanning per channel, call for scanning next channel
      break;
    case WiFiState_e::STA_Connecting:
    case WiFiState_e::STA_Reconnecting:

      // Check if (re)connecting has finished
    {
      const STA_connected_state sta_connected_state = getSTA_connected_state();

      if (sta_connected_state == STA_connected_state::Connected) {
        setState(WiFiState_e::STA_Connected);
      } else if (_state_timeout.timeReached()) {
        if (_state == WiFiState_e::STA_Connecting) {
          setState(WiFiState_e::STA_Reconnecting, WIFI_STATE_MACHINE_STA_CONNECTING_TIMEOUT);
        } else {
          setState(WiFiState_e::WiFiOFF);
        }
      }

      break;
    }
    case WiFiState_e::STA_Connected:

      // Check if still connected
      if (getSTA_connected_state() != STA_connected_state::Connected) {
        setState(WiFiState_e::WiFiOFF);
      } else {
        // Else mark last timestamp seen as connected
        _last_seen_connected.setNow();
      }
      break;
  }


  {
    // Check if we need to start AP
    // Flag captive portal in webserver and/or whether we might be in setup mode
  }
}

bool ESPEasyWiFi_t::connected() const
{
  return getSTA_connected_state() == STA_connected_state::Connected;
}

void ESPEasyWiFi_t::disconnect() { WiFi.disconnect(Settings.WiFiRestart_connection_lost()); }

void ESPEasyWiFi_t::setState(WiFiState_e newState, uint32_t timeout) {
  if (loglevelActiveFor(LOG_LEVEL_INFO)) {
    addLog(
      LOG_LEVEL_INFO,
      concat(F("WiFi : Set state from: "), toString(_state)) +
      concat(F(" to: "),                   toString(newState)) +
      concat(F(" timeout: "),              timeout));
  }

  if (timeout == 0)
  {
    _state_timeout.clear();

  } else {
    _state_timeout.setMillisFromNow(timeout);
  }

  _last_state_change.setNow();
  _state = newState;


  switch (newState)
  {
    case WiFiState_e::Disabled:
      // Do nothing here, as the device is disabled.
      break;
    case WiFiState_e::WiFiOFF:
      // TODO TD-er: Must cancel all and turn off WiFi.
      break;
    case WiFiState_e::AP_only:
      break;
    case WiFiState_e::IdleWaiting:
      // Do nothing here as we're waiting till the timeout is over
      break;
    case WiFiState_e::STA_Scanning:
      // Start scanning
      startScanning();
      break;
    case WiFiState_e::STA_AP_Scanning:
      // Start scanning per channel
      break;
    case WiFiState_e::STA_Connecting:
    case WiFiState_e::STA_Reconnecting:

      // Start connecting
      if (!connectSTA()) {
        // TODO TD-er: Must keep track of failed attempts and start AP when either no credentials present or nr. of attempts failed > some
        // threshold.
        setState(WiFiState_e::IdleWaiting, 100);
      }
      break;
    case WiFiState_e::STA_Connected:
      _last_seen_connected.setNow();
      _state_timeout.clear();
      break;
  }
}

void ESPEasyWiFi_t::checkConnectProgress() {}

void ESPEasyWiFi_t::startScanning()
{
  _state = WiFiState_e::STA_Scanning;
  setSTA(true);
  WifiScan(true);
  _last_state_change.setNow();
}

bool ESPEasyWiFi_t::connectSTA()
{
  if (!WiFi_AP_Candidates.hasCandidateCredentials())
  {
    if (!WiFiEventData.warnedNoValidWiFiSettings)
    {
      addLog(LOG_LEVEL_ERROR, F("WIFI : No valid wifi settings"));
      WiFiEventData.warnedNoValidWiFiSettings = true;
    }
    WiFiEventData.last_wifi_connect_attempt_moment.clear();
    WiFiEventData.wifi_connect_attempt     = 1;
    WiFiEventData.wifiConnectAttemptNeeded = false;

    // No need to wait longer to start AP mode.
    if (!Settings.DoNotStartAP())
    {
      setAP(true);
    }
    return false;
  }

  if (WiFiEventData.lastDisconnectReason != WIFI_DISCONNECT_REASON_UNSPECIFIED) {
    addLog(LOG_LEVEL_INFO, concat(
             F("WiFi : Disconnect reason: "),
             getLastDisconnectReason()));
    WiFiEventData.processedDisconnect = true;
  }

  WiFiEventData.warnedNoValidWiFiSettings = false;
  setSTA(true);
# if defined(ESP8266)
  wifi_station_set_hostname(NetworkCreateRFCCompliantHostname().c_str());

# endif // if defined(ESP8266)
# if defined(ESP32)

  //  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
# endif // if defined(ESP32)
  setConnectionSpeed();
  setupStaticIPconfig();

  // Start the process of connecting or starting AP
  if (!WiFi_AP_Candidates.getNext(true))
  {
    return false;
  }

  const WiFi_AP_Candidate candidate = WiFi_AP_Candidates.getCurrent();

  if (loglevelActiveFor(LOG_LEVEL_INFO)) {
    addLogMove(LOG_LEVEL_INFO, strformat(
                 F("WIFI : Connecting %s attempt #%u"),
                 candidate.toString().c_str(),
                 _connect_attempt));
  }
  WiFiEventData.markWiFiBegin();

  if (prepareWiFi()) {
    setNetworkMedium(NetworkMedium_t::WIFI);
    RTC.clearLastWiFi();
    RTC.lastWiFiSettingsIndex = candidate.index;

    float tx_pwr = 0; // Will be set higher based on RSSI when needed.
    // FIXME TD-er: Must check WiFiEventData.wifi_connect_attempt to increase TX power
# if FEATURE_SET_WIFI_TX_PWR

    if (Settings.UseMaxTXpowerForSending()) {
      tx_pwr = Settings.getWiFi_TX_power();
    }
    SetWiFiTXpower(tx_pwr, candidate.rssi);
# endif // if FEATURE_SET_WIFI_TX_PWR

    // Start connect attempt now, so no longer needed to attempt new connection.
    WiFiEventData.wifiConnectAttemptNeeded = false;
    WiFiEventData.wifiConnectInProgress    = true;
    const String key = WiFi_AP_CandidatesList::get_key(candidate.index);

# if FEATURE_USE_IPV6

    if (Settings.EnableIPv6()) {
      WiFi.enableIPv6(true);
    }
# endif // if FEATURE_USE_IPV6

# ifdef ESP32

    if (Settings.IncludeHiddenSSID()) {
      setWiFiCountryPolicyManual();
    }
# endif // ifdef ESP32

    if (candidate.bits.isHidden /*&& Settings.HiddenSSID_SlowConnectPerBSSID()*/) {
      //      WiFi.disconnect(false, true);
# ifdef ESP32
      WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
# endif
      delay(100);
      WiFi.begin(candidate.ssid.c_str(), key.c_str(), candidate.channel, candidate.bssid.mac);

      // If the ssid returned from the scan is empty, it is a hidden SSID
      // it appears that the WiFi.begin() function is asynchronous and takes
      // additional time to connect to a hidden SSID. Therefore a delay of 1000ms
      // is added for hidden SSIDs before calling WiFi.status()
      delay(1000);

      //      WiFi.waitForConnectResult(6000);
    } else {
      if (candidate.allowQuickConnect()) {
# ifdef ESP32
        WiFi.setScanMethod(WIFI_FAST_SCAN);
# endif
        WiFi.begin(candidate.ssid.c_str(), key.c_str(), candidate.channel, candidate.bssid.mac);
      } else {
# ifdef ESP32
        WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
# endif
        WiFi.begin(candidate.ssid.c_str(), key.c_str());
      }
    }

    // Always wait for a second
    WiFi.waitForConnectResult(1000); // https://github.com/arendst/Tasmota/issues/14985

  }

  return true;
}

} // namespace wifi
} // namespace net
} // namespace ESPEasy

#endif // if FEATURE_WIFI
