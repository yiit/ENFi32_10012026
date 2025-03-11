#include "../ESPEasyCore/ESPEasyWifi_abstracted.h"

#if FEATURE_WIFI

# include "../Globals/EventQueue.h"
# include "../Globals/Settings.h"
# include "../Globals/ESPEasyWiFiEvent.h"

# include "../Helpers/StringConverter.h"

namespace ESPEasy {
namespace net {
namespace wifi {

const __FlashStringHelper* getWifiModeString(WiFiMode_t wifimode)
{
  switch (wifimode)
  {
    case WIFI_OFF:   return F("OFF");
    case WIFI_STA:   return F("STA");
    case WIFI_AP:    return F("AP");
    case WIFI_AP_STA: return F("AP+STA");
    default:
      break;
  }
  return F("Unknown");
}

bool WiFiConnected()     { return ESPEasyWiFi.connected(); }

bool setSTA(bool enable) { return setSTA_AP(enable,  WifiIsAP(WiFi.getMode())); }

bool setAP(bool enable)  { return setSTA_AP(WifiIsSTA(WiFi.getMode()), enable); }

bool setSTA_AP(bool sta_enable, bool ap_enable)
{
  if (ap_enable) {
    return setWifiMode(sta_enable ? WIFI_AP_STA : WIFI_AP);
  }
  return setWifiMode(sta_enable ? WIFI_STA : WIFI_OFF);
}

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

  doSetWiFiTXpower(dBm);

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

int GetRSSI_quality() {
  long rssi = WiFi.RSSI();

  if (-50 < rssi) { return 10; }

  if (rssi <= -98) { return 0;  }
  rssi = rssi + 97; // Range 0..47 => 1..9
  return (rssi / 5) + 1;
}

void setConnectionSpeed() { setConnectionSpeed(Settings.ForceWiFi_bg_mode()); }

} // namespace wifi
} // namespace net
} // namespace ESPEasy


#endif // if FEATURE_WIFI
