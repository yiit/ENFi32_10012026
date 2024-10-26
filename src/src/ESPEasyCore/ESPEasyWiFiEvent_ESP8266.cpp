#include "../ESPEasyCore/ESPEasyWiFiEvent_ESP8266.h"

#ifdef ESP8266

#include "../DataStructs/RTCStruct.h"

#include "../DataTypes/ESPEasyTimeSource.h"

#include "../ESPEasyCore/ESPEasyEth.h"
#include "../ESPEasyCore/ESPEasy_Log.h"
#include "../ESPEasyCore/ESPEasyNetwork.h"
#include "../ESPEasyCore/ESPEasyWifi.h"
#include "../ESPEasyCore/ESPEasyWifi_ProcessEvent.h"

#include "../Globals/ESPEasyWiFiEvent.h"
#include "../Globals/NetworkState.h"
#include "../Globals/RTC.h"
#include "../Globals/WiFi_AP_Candidates.h"

#include "../Helpers/ESPEasy_time_calc.h"


void WiFi_Access_Static_IP::set_use_static_ip(bool enabled) {
  _useStaticIp = enabled;
}
void setUseStaticIP(bool enabled) {
  WiFi_Access_Static_IP tmp_wifi;

  tmp_wifi.set_use_static_ip(enabled);
}





// ********************************************************************************
// Functions called on events.
// Make sure not to call anything in these functions that result in delay() or yield()
// ********************************************************************************
void onConnected(const WiFiEventStationModeConnected& event) {
  WiFiEventData.markConnected(event.ssid, event.bssid, event.channel);
}

void onDisconnect(const WiFiEventStationModeDisconnected& event) {
  WiFiEventData.markDisconnect(event.reason);
  if (WiFi.status() == WL_CONNECTED) {
    // See https://github.com/esp8266/Arduino/issues/5912
    WiFi.persistent(false);
    WiFi.disconnect(false);
    delay(0);
  }
}

void onGotIP(const WiFiEventStationModeGotIP& event) {
  WiFiEventData.markGotIP();
}

void onDHCPTimeout() {
  WiFiEventData.processedDHCPTimeout = false;
}

void onConnectedAPmode(const WiFiEventSoftAPModeStationConnected& event) {
  WiFiEventData.markConnectedAPmode(event.mac);
}

void onDisconnectedAPmode(const WiFiEventSoftAPModeStationDisconnected& event) {
  WiFiEventData.markDisconnectedAPmode(event.mac);
}

void onStationModeAuthModeChanged(const WiFiEventStationModeAuthModeChanged& event) {
  WiFiEventData.setAuthMode(event.newMode);
}

#if FEATURE_ESP8266_DIRECT_WIFI_SCAN
void onWiFiScanDone(void *arg, STATUS status) {
  if (status == OK) {
    auto *head = reinterpret_cast<bss_info *>(arg);
    int scanCount = 0;
    for (bss_info *it = head; it != nullptr; it = STAILQ_NEXT(it, next)) {
      WiFi_AP_Candidates.process_WiFiscan(*it);
      ++scanCount;
    }
    WiFi_AP_Candidates.after_process_WiFiscan();
    WiFiEventData.lastGetScanMoment.setNow();
//    WiFiEventData.processedScanDone = true;
# ifndef BUILD_NO_DEBUG
/*
    if (loglevelActiveFor(LOG_LEVEL_INFO)) {
      addLogMove(LOG_LEVEL_INFO, concat(F("WiFi : Scan finished (ESP8266), found: "), scanCount));
    }
*/
#endif
    WiFi_AP_Candidates.load_knownCredentials();
    if (WiFi_AP_Candidates.addedKnownCandidate() || !NetworkConnected()) {
      WiFiEventData.wifiConnectAttemptNeeded = true;
      # ifndef BUILD_NO_DEBUG
      if (WiFi_AP_Candidates.addedKnownCandidate()) {
        //addLog(LOG_LEVEL_INFO, F("WiFi : Added known candidate, try to connect"));
      }
      #endif
      NetworkConnectRelaxed();
    }

  }

  WiFiMode_t mode = WiFi.getMode();
  setWifiMode(WIFI_OFF);
  delay(1);
  setWifiMode(mode);
  delay(1);
}

#endif 

#endif
