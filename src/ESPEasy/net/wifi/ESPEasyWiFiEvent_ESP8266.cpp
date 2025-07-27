#include "../wifi/ESPEasyWiFiEvent_ESP8266.h"

#if FEATURE_WIFI
# ifdef ESP8266

#  include "../../../src/DataStructs/RTCStruct.h"
#  include "../../../src/DataTypes/ESPEasyTimeSource.h"
#  include "../../../src/ESPEasyCore/ESPEasy_Log.h"
#  include "../../../src/Globals/RTC.h"
#  include "../../../src/Globals/WiFi_AP_Candidates.h"
#  include "../../../src/Helpers/ESPEasy_time_calc.h"

#  include "../ESPEasyNetwork.h"
#  include "../wifi/ESPEasyWifi.h"
#  include "../wifi/ESPEasyWifi_ProcessEvent.h"

#  include "../Globals/ESPEasyWiFiEvent.h"
#  include "../Globals/NetworkState.h"


namespace ESPEasy {
namespace net {
namespace wifi {

void WiFi_Access_Static_IP::set_use_static_ip(bool enabled) { _useStaticIp = enabled; }

void setUseStaticIP(bool enabled) {
  WiFi_Access_Static_IP tmp_wifi;

  tmp_wifi.set_use_static_ip(enabled);
}

// ********************************************************************************
// Functions called on events.
// Make sure not to call anything in these functions that result in delay() or yield()
// ********************************************************************************
void onConnected(const WiFiEventStationModeConnected& event) { WiFiEventData.markConnected(event.ssid, event.bssid, event.channel); }

void onDisconnect(const WiFiEventStationModeDisconnected& event) {
  WiFiEventData.markDisconnect(event.reason);

  if (WiFi.status() == WL_CONNECTED) {
    // See https://github.com/esp8266/Arduino/issues/5912
    WiFi.persistent(false);
    WiFi.disconnect(false);
    delay(0);
  }
}

void onGotIP(const WiFiEventStationModeGotIP& event)
{
  WiFiEventData.markGotIP(
    event.ip,
    event.mask,
    event.gw);
}

void onDHCPTimeout()                                                                { WiFiEventData.processedDHCPTimeout = false; }

void onConnectedAPmode(const WiFiEventSoftAPModeStationConnected& event)            { WiFiEventData.markConnectedAPmode(event.mac); }

void onDisconnectedAPmode(const WiFiEventSoftAPModeStationDisconnected& event)      { WiFiEventData.markDisconnectedAPmode(event.mac); }

void onStationModeAuthModeChanged(const WiFiEventStationModeAuthModeChanged& event) { WiFiEventData.setAuthMode(event.newMode); }

#  if FEATURE_ESP8266_DIRECT_WIFI_SCAN

void onWiFiScanDone(void *arg, STATUS status) {
  if (status == OK) {
    auto *head      = reinterpret_cast<bss_info *>(arg);
    int   scanCount = 0;

    for (bss_info *it = head; it != nullptr; it = STAILQ_NEXT(it, next)) {
      WiFi_AP_Candidates.process_WiFiscan(*it);
      ++scanCount;
    }
    WiFi_AP_Candidates.after_process_WiFiscan();
    WiFiEventData.lastGetScanMoment.setNow();

    //    WiFiEventData.processedScanDone = true;
#   ifndef BUILD_NO_DEBUG

    /*
        if (loglevelActiveFor(LOG_LEVEL_INFO)) {
          addLogMove(LOG_LEVEL_INFO, concat(F("WiFi : Scan finished (ESP8266), found: "), scanCount));
        }
     */
#   endif // ifndef BUILD_NO_DEBUG
    WiFi_AP_Candidates.load_knownCredentials();

    if (WiFi_AP_Candidates.addedKnownCandidate() || !NetworkConnected()) {
      WiFiEventData.wifiConnectAttemptNeeded = true;
      #   ifndef BUILD_NO_DEBUG

      if (WiFi_AP_Candidates.addedKnownCandidate()) {
        // addLog(LOG_LEVEL_INFO, F("WiFi : Added known candidate, try to connect"));
      }
      #   endif // ifndef BUILD_NO_DEBUG
      NetworkConnectRelaxed();
    }

  }

  WiFiMode_t mode = WiFi.getMode();
  ESPEasy::net::wifi::setWifiMode(WIFI_OFF);
  delay(1);
  ESPEasy::net::wifi::setWifiMode(mode);
  delay(1);
}

#  endif // if FEATURE_ESP8266_DIRECT_WIFI_SCAN

} // namespace wifi
} // namespace net
} // namespace ESPEasy

# endif // ifdef ESP8266
#endif // if FEATURE_WIFI
