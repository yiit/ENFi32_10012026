#include "../ESPEasyCore/ESPEasyWifi_abstracted.h"

#if FEATURE_WIFI
# ifdef ESP32

#  include "../Globals/ESPEasyWiFiEvent.h"
#  include "../Globals/EventQueue.h"
#  include "../Globals/Settings.h"
#  include "../Globals/WiFi_AP_Candidates.h"
#  include "../ESPEasyCore/ESPEasyWiFiEvent_ESP32.h"

#  include "../Helpers/StringConverter.h"


#  include "../ESPEasyCore/ESPEasyNetwork.h" // Needed for NetworkCreateRFCCompliantHostname, WiFi code should not include network code
#  include "../ESPEasyCore/ESPEasyWifi.h"    // Needed for WifiDisconnect. Maybe this should be moved to these 'abstract' files?

#  include "../ESPEasyCore/ESPEasyWifi_ProcessEvent.h"


#  include <WiFiGeneric.h>
#  include <esp_wifi.h> // Needed to call ESP-IDF functions like esp_wifi_....

#  include <esp_phy_init.h>


namespace ESPEasy {
namespace net {
namespace wifi {

bool WiFi_pre_setup() {

  registerWiFiEventHandler();
  WiFi.persistent(false);

  return setSTA_AP(false, false);
}

bool WiFi_pre_STA_setup()
{
  if (!setSTA(true)) { return false; }

  WiFi.setAutoReconnect(false);
  delay(10);
  return true;
}

void WiFiDisconnect() {
  //  removeWiFiEventHandler();
  WiFi.disconnect();
  delay(100);
  {
    const IPAddress ip;
    const IPAddress gw;
    const IPAddress subnet;
    const IPAddress dns;
    WiFi.config(ip, gw, subnet, dns);
  }
}

bool WifiIsAP(WiFiMode_t wifimode)  { return (wifimode == WIFI_MODE_AP) || (wifimode == WIFI_MODE_APSTA); }

bool WifiIsSTA(WiFiMode_t wifimode) { return (wifimode & WIFI_MODE_STA) != 0; }

bool setWifiMode(WiFiMode_t new_mode)
{
  const WiFiMode_t cur_mode = WiFi.getMode();

  // Made this static flag an int as ESP8266 and ESP32 differ in the "not set" values
  static int8_t processing_wifi_mode = -1;

  if (cur_mode == new_mode) {
    if (cur_mode != WIFI_OFF) {
      registerWiFiEventHandler();
    }
    return true;
  }

  if (processing_wifi_mode == static_cast<int8_t>(new_mode)) {
    // Prevent loops
    return true;
  }
  processing_wifi_mode = static_cast<int8_t>(new_mode);


  if (cur_mode == WIFI_OFF) {
    registerWiFiEventHandler();

    // Needs to be set while WiFi is off
    WiFi.hostname(NetworkCreateRFCCompliantHostname());
    WiFiEventData.markWiFiTurnOn();
  }

  if (new_mode != WIFI_OFF) {
    #  ifdef ESP8266

    // See: https://github.com/esp8266/Arduino/issues/6172#issuecomment-500457407
    WiFi.forceSleepWake(); // Make sure WiFi is really active.
    #  endif // ifdef ESP8266
    delay(100);
  } else {
    WifiDisconnect();

    //    delay(100);
    processDisconnect();
    WiFiEventData.clear_processed_flags();
    removeWiFiEventHandler();
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

    // FIXME TD-er: Is this correct to mark Turn ON ????
    WiFiEventData.markWiFiTurnOn();

    // Needs to be set while WiFi is off
    WiFi.hostname(NetworkCreateRFCCompliantHostname());
    delay(100);
    esp_wifi_set_ps(WIFI_PS_NONE);

    //    esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
    delay(1);
    #  ifdef ESP32
    WiFi.STA.end();
    #  endif
  } else {
    /*
        if (cur_mode == WIFI_OFF) {
          registerWiFiEventHandler();
        }
     */

    // Only set power mode when AP is not enabled
    // When AP is enabled, the sleep mode is already set to WIFI_NONE_SLEEP
    if (!WifiIsAP(new_mode)) {
      if (Settings.WifiNoneSleep()) {
        setWiFiNoneSleep();
      } else if (Settings.EcoPowerMode()) {
        setWiFiEcoPowerMode();
      } else {
        // Default
        setWiFiDefaultPowerMode();
      }
    }
#  if FEATURE_SET_WIFI_TX_PWR
    SetWiFiTXpower();
#  endif

    if (WifiIsSTA(new_mode)) {
      //      WiFi.setAutoConnect(Settings.SDK_WiFi_autoreconnect());
      WiFi.setAutoReconnect(Settings.SDK_WiFi_autoreconnect());
    }
    delay(100); // Must allow for some time to init.
  }
  const bool new_mode_AP_enabled =  WifiIsAP(new_mode);

  if (WifiIsAP(cur_mode) && !new_mode_AP_enabled) {
    eventQueue.add(F("WiFi#APmodeDisabled"));
  }

  if (WifiIsAP(cur_mode) != new_mode_AP_enabled) {
    // Mode has changed
    setAPinternal(new_mode_AP_enabled);
  }
  #  if FEATURE_MDNS
  #   ifdef ESP8266

  // notifyAPChange() is not present in the ESP32 MDNSResponder
  MDNS.notifyAPChange();
  #   endif // ifdef ESP8266
  #  endif // if FEATURE_MDNS
  return true;
}

void removeWiFiEventHandler()
{
  WiFi.removeEvent(WiFiEventData.wm_event_id);
  WiFiEventData.wm_event_id = 0;
}

void registerWiFiEventHandler()
{
  if (WiFiEventData.wm_event_id != 0) {
    removeWiFiEventHandler();
  }
  WiFiEventData.wm_event_id = WiFi.onEvent(WiFiEvent);
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
    case WiFiConnectionProtocol::WiFi_Protocol_HT20:
    case WiFiConnectionProtocol::WiFi_Protocol_HT40:
    case WiFiConnectionProtocol::WiFi_Protocol_HE20:

      threshold = WIFI_SENSITIVITY_n;

      if (maxTXpwr > MAX_TX_PWR_DBM_n) { maxTXpwr = MAX_TX_PWR_DBM_n; }
      break;
#  if ESP_IDF_VERSION > ESP_IDF_VERSION_VAL(5, 2, 0)
    case WiFiConnectionProtocol::WiFi_Protocol_11a:
    case WiFiConnectionProtocol::WiFi_Protocol_VHT20:
      // FIXME TD-er: Must determine max. TX power for these 5 GHz modi
#  endif // if ESP_IDF_VERSION > ESP_IDF_VERSION_VAL(5, 2, 0)
    case WiFiConnectionProtocol::WiFi_Protocol_LR:
    case WiFiConnectionProtocol::Unknown:
      break;
  }
  return threshold;
}

WiFiConnectionProtocol getConnectionProtocol()
{
  if (WiFi.RSSI() < 0) {
    wifi_phy_mode_t phymode;
    esp_wifi_sta_get_negotiated_phymode(&phymode);

    switch (phymode)
    {
      case WIFI_PHY_MODE_11B:   return WiFiConnectionProtocol::WiFi_Protocol_11b;
      case WIFI_PHY_MODE_11G:   return WiFiConnectionProtocol::WiFi_Protocol_11g;
      case WIFI_PHY_MODE_HT20:  return WiFiConnectionProtocol::WiFi_Protocol_HT20;
      case WIFI_PHY_MODE_HT40:  return WiFiConnectionProtocol::WiFi_Protocol_HT40;
      case WIFI_PHY_MODE_HE20:  return WiFiConnectionProtocol::WiFi_Protocol_HE20;
      case WIFI_PHY_MODE_LR:    return WiFiConnectionProtocol::WiFi_Protocol_LR;
#  if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)

      // 5 GHz
      case WIFI_PHY_MODE_11A:   return WiFiConnectionProtocol::WiFi_Protocol_11a;
      case WIFI_PHY_MODE_VHT20: return WiFiConnectionProtocol::WiFi_Protocol_VHT20;
#  endif // if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
    }
  }
  return WiFiConnectionProtocol::Unknown;
}

#  if FEATURE_SET_WIFI_TX_PWR

void doSetWiFiTXpower(float& dBm)
{
  int8_t power = dBm * 4;

  esp_wifi_set_max_tx_power(power);

  if (esp_wifi_get_max_tx_power(&power) == ESP_OK)  {
    dBm = static_cast<float>(power) / 4.0f;
  }
}

#  endif // if FEATURE_SET_WIFI_TX_PWR

void setConnectionSpeed(bool ForceWiFi_bg_mode) {
  // Does not (yet) work, so commented out.

  // HT20 = 20 MHz channel width.
  // HT40 = 40 MHz channel width.
  // In theory, HT40 can offer upto 150 Mbps connection speed.
  // However since HT40 is using nearly all channels on 2.4 GHz WiFi,
  // Thus you are more likely to experience disturbances.
  // The response speed and stability is better at HT20 for ESP units.
  esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT20);

  uint8_t protocol = WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G; // Default to BG

  if (!ForceWiFi_bg_mode || (WiFiEventData.connectionFailures > 10)) {
    // Set to use BGN
    protocol |= WIFI_PROTOCOL_11N;
    #  ifdef ESP32C6
    protocol |= WIFI_PROTOCOL_11AX;
    #  endif
  }

  const WiFi_AP_Candidate candidate = WiFi_AP_Candidates.getCurrent();

  if (candidate.phy_known()) {
    // Check to see if the access point is set to "N-only"
    if ((protocol & WIFI_PROTOCOL_11N) == 0) {
      if (!candidate.bits.phy_11b && !candidate.bits.phy_11g && candidate.bits.phy_11n) {
        if (candidate.bits.phy_11n) {
          // Set to use BGN
          protocol |= WIFI_PROTOCOL_11N;
          addLog(LOG_LEVEL_INFO, F("WIFI : AP is set to 802.11n only"));
        }
#  ifdef ESP32C6

        if (candidate.bits.phy_11ax) {
          // Set to use WiFi6
          protocol |= WIFI_PROTOCOL_11AX;
          addLog(LOG_LEVEL_INFO, F("WIFI : AP is set to 802.11ax"));
        }
#  endif // ifdef ESP32C6
      }
    }
  }


  if (WifiIsSTA(WiFi.getMode())) {
    // Set to use "Long GI" making it more resilliant to reflections
    // See: https://www.tp-link.com/us/configuration-guides/q_a_basic_wireless_concepts/?configurationId=2958#_idTextAnchor038
    esp_wifi_config_80211_tx_rate(WIFI_IF_STA, WIFI_PHY_RATE_MCS3_LGI);
    esp_wifi_set_protocol(WIFI_IF_STA, protocol);
  }

  if (WifiIsAP(WiFi.getMode())) {
    esp_wifi_set_protocol(WIFI_IF_AP, protocol);
  }
}

void setWiFiNoneSleep() { WiFi.setSleep(WIFI_PS_NONE); }

void setWiFiEcoPowerMode()
{
  // Maximum modem power saving.
  // In this mode, interval to receive beacons is determined by the listen_interval parameter in wifi_sta_config_t
  // FIXME TD-er: Must test if this is desired behavior in ESP32.
  WiFi.setSleep(WIFI_PS_MAX_MODEM);
}

void setWiFiDefaultPowerMode()
{
  // Minimum modem power saving.
  // In this mode, station wakes up to receive beacon every DTIM period
  WiFi.setSleep(WIFI_PS_MIN_MODEM);
}

void setWiFiCountryPolicyManual()
{
  /*   wifi_country_t config = {
      .cc     = "01",
      .schan  = 1,
      .nchan  = 14,
      .policy = WIFI_COUNTRY_POLICY_MANUAL,
     };

     esp_wifi_set_country(&config);
   */
}

} // namespace wifi
} // namespace net
} // namespace ESPEasy

# endif // ifdef ESP32
#endif // if FEATURE_WIFI
