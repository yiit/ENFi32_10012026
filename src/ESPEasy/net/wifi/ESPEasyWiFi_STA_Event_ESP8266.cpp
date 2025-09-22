#include "../wifi/ESPEasyWiFi_STA_Event_ESP8266.h"

#if FEATURE_WIFI
# ifdef ESP8266

#  include "../../../src/DataStructs/RTCStruct.h"
#  include "../../../src/DataTypes/ESPEasyTimeSource.h"
#  include "../../../src/ESPEasyCore/ESPEasy_Log.h"
#  include "../../../src/Globals/RTC.h"
#  include "../../../src/Helpers/ESPEasy_time_calc.h"
#  include "../../../src/Helpers/StringGenerator_WiFi.h"


#  include "../ESPEasyNetwork.h"
#  include "../wifi/ESPEasyWifi.h"
#  include "../wifi/ESPEasyWifi_ProcessEvent.h"

#  include "../Globals/ESPEasyWiFiEvent.h"
#  include "../Globals/NetworkState.h"
#  include "../Globals/WiFi_AP_Candidates.h"


namespace ESPEasy {
namespace net {
namespace wifi {

static NWPluginData_static_runtime stats_and_cache(false, "WiFi"); // Cannot use flash strings during init of static objects
static WiFiDisconnectReason _wifi_disconnect_reason = WiFiDisconnectReason::WIFI_DISCONNECT_REASON_UNSPECIFIED;

static uint8_t _enc_type{};

static bool _ESPEasyWiFi_STA_EventHandler_initialized{};

ESPEasyWiFi_STA_EventHandler::ESPEasyWiFi_STA_EventHandler(networkIndex_t networkIndex)
{
  stats_and_cache.clear(networkIndex);

  // WiFi event handlers

  if (!_ESPEasyWiFi_STA_EventHandler_initialized) {
    stationConnectedHandler                   = WiFi.onStationModeConnected(onConnected);
    stationDisconnectedHandler                = WiFi.onStationModeDisconnected(onDisconnect);
    stationGotIpHandler                       = WiFi.onStationModeGotIP(onGotIP);
    stationModeDHCPTimeoutHandler             = WiFi.onStationModeDHCPTimeout(onDHCPTimeout);
    stationModeAuthModeChangeHandler          = WiFi.onStationModeAuthModeChanged(onStationModeAuthModeChanged);
    APModeStationConnectedHandler             = WiFi.onSoftAPModeStationConnected(onConnectedAPmode);
    APModeStationDisconnectedHandler          = WiFi.onSoftAPModeStationDisconnected(onDisconnectedAPmode);
    _ESPEasyWiFi_STA_EventHandler_initialized = true;
  }
}

ESPEasyWiFi_STA_EventHandler::~ESPEasyWiFi_STA_EventHandler()
{
  stats_and_cache.processEvent_and_clear();
}

bool ESPEasyWiFi_STA_EventHandler::initialized()                                            {
  return _ESPEasyWiFi_STA_EventHandler_initialized;
}

NWPluginData_static_runtime* ESPEasyWiFi_STA_EventHandler::getNWPluginData_static_runtime() { return &stats_and_cache; }

WiFiDisconnectReason         ESPEasyWiFi_STA_EventHandler::getLastDisconnectReason() const  { return _wifi_disconnect_reason; }

uint8_t                      ESPEasyWiFi_STA_EventHandler::getAuthMode() const              { return _enc_type; }

const __FlashStringHelper *  ESPEasyWiFi_STA_EventHandler::getWiFi_encryptionType() const
{
  return WiFi_encryptionType(_enc_type);
}

// ********************************************************************************
// Functions called on events.
// Make sure not to call anything in these functions that result in delay() or yield()
// ********************************************************************************
void ESPEasyWiFi_STA_EventHandler::onConnected(const WiFiEventStationModeConnected& event) {
  stats_and_cache.mark_connected();

  _enc_type = WiFi_AP_Candidates.getCurrent().enc_type;
}

void ESPEasyWiFi_STA_EventHandler::onDisconnect(const WiFiEventStationModeDisconnected& event) {
  stats_and_cache.mark_disconnected();
  stats_and_cache.mark_lost_IP(); // ESP8266 doesn't have a separate event for lost IP
  _wifi_disconnect_reason = event.reason;

  if (WiFi.status() == WL_CONNECTED) {
    // See https://github.com/esp8266/Arduino/issues/5912
    WiFi.persistent(false);
    WiFi.disconnect(false);
    delay(0);
  }
}

void ESPEasyWiFi_STA_EventHandler::onGotIP(const WiFiEventStationModeGotIP& event)
{
  stats_and_cache.mark_got_IP();

  // WiFiEventData.markGotIP(event.ip, event.mask, event.gw);
}

void ESPEasyWiFi_STA_EventHandler::onDHCPTimeout()                                                                {
  //  WiFiEventData.processedDHCPTimeout = false;
}

void ESPEasyWiFi_STA_EventHandler::onConnectedAPmode(const WiFiEventSoftAPModeStationConnected& event)            {
  //  WiFiEventData.markConnectedAPmode(event.mac);
}

void ESPEasyWiFi_STA_EventHandler::onDisconnectedAPmode(const WiFiEventSoftAPModeStationDisconnected& event)      {
  //  WiFiEventData.markDisconnectedAPmode(event.mac);
}

void ESPEasyWiFi_STA_EventHandler::onStationModeAuthModeChanged(const WiFiEventStationModeAuthModeChanged& event) {
  _enc_type = event.newMode;

  //  WiFiEventData.setAuthMode(event.newMode);
}

#  if FEATURE_ESP8266_DIRECT_WIFI_SCAN

void ESPEasyWiFi_STA_EventHandler::onWiFiScanDone(void ESPEasyWiFi_STA_EventHandler::*arg, STATUS status) {
  if (status == OK) {
    auto *head      = reinterpret_cast<bss_info *>(arg);
    int   scanCount = 0;

    for (bss_info *it = head; it != nullptr; it = STAILQ_NEXT(it, next)) {
      WiFi_AP_Candidates.process_WiFiscan(*it);
      ++scanCount;
    }
    WiFi_AP_Candidates.after_process_WiFiscan();

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
      //      NetworkConnectRelaxed();
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
