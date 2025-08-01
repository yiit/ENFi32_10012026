#include "../NWPluginStructs/NW001_data_struct_WiFi_STA.h"

#ifdef USES_NW001

# include "../../../src/Globals/Settings.h"

# include "../../../src/Helpers/ESPEasy_time_calc.h"
# include "../../../src/Helpers/LongTermOnOffTimer.h"
# include "../../../src/Helpers/StringConverter.h"

# include "../wifi/ESPEasyWifi.h"
# include "../wifi/WiFiDisconnectReason.h"

# define NW_PLUGIN_ID  1

namespace ESPEasy {
namespace net {
namespace wifi {

static LongTermOnOffTimer _startStopStats;
static LongTermOnOffTimer _connectedStats;
static LongTermOnOffTimer _gotIPStats;
# ifdef ESP32
#  if FEATURE_USE_IPV6
static LongTermOnOffTimer _gotIP6Stats;
#  endif
static IPAddress _dns_cache[2]{};
static wifi_event_sta_connected_t _wifi_event_sta_connected;
static WiFiDisconnectReason _wifi_disconnect_reason = WiFiDisconnectReason::WIFI_DISCONNECT_REASON_UNSPECIFIED;
# endif // ifdef ESP32

NW001_data_struct_WiFi_STA::NW001_data_struct_WiFi_STA(networkIndex_t networkIndex)
  : NWPluginData_base(nwpluginID_t(NW_PLUGIN_ID), networkIndex)
{
  _connectedStats.clear();
  _gotIPStats.clear();
# ifdef ESP32
#  if FEATURE_USE_IPV6
  _gotIP6Stats.clear();
#  endif
  nw_event_id = Network.onEvent(NW001_data_struct_WiFi_STA::onEvent);
  memset(&_wifi_event_sta_connected, 0, sizeof(_wifi_event_sta_connected));
# endif // ifdef ESP32
}

NW001_data_struct_WiFi_STA::~NW001_data_struct_WiFi_STA()
{
# ifdef ESP32

  if (nw_event_id != 0) {
    Network.removeEvent(nw_event_id);
  }
  nw_event_id = 0;
# endif // ifdef ESP32
}

void NW001_data_struct_WiFi_STA::webform_load(EventStruct *event) {}

void NW001_data_struct_WiFi_STA::webform_save(EventStruct *event) {}

bool NW001_data_struct_WiFi_STA::webform_getPort(String& str)     { return true; }

bool NW001_data_struct_WiFi_STA::init(EventStruct *event)
{
  /*
     // Taken from ESPEasy_setup()
     // Might no longer be needed as we now have a proper WiFi state machine.

   # ifdef ESP32

      // FIXME TD-er: Disabled for now, as this may not return and thus block the ESP forever.
      // See: https://github.com/espressif/esp-idf/issues/15862
      // check_and_update_WiFi_Calibration();
   # endif // ifdef ESP32

      ESPEasy::net::wifi::WiFi_AP_Candidates.clearCache();
      ESPEasy::net::wifi::WiFi_AP_Candidates.load_knownCredentials();
      ESPEasy::net::wifi::setSTA(true);

      if (!ESPEasy::net::wifi::WiFi_AP_Candidates.hasCandidateCredentials()) {
        WiFiEventData.wifiSetup = true;
        RTC.clearLastWiFi(); // Must scan all channels
        // Wait until scan has finished to make sure as many as possible are found
        // We're still in the setup phase, so nothing else is taking resources of the ESP.
        ESPEasy::net::wifi::WifiScan(false);
        WiFiEventData.lastScanMoment.clear();
      }



      // Always perform WiFi scan
      // It appears reconnecting from RTC may take just as long to be able to send first packet as performing a scan first and then
   #connect.
      // Perhaps the WiFi radio needs some time to stabilize first?
      if (!ESPEasy::net::wifi::WiFi_AP_Candidates.hasCandidates()) {
        ESPEasy::net::wifi::WifiScan(false, RTC.lastWiFiChannel);
      }
      ESPEasy::net::wifi::WiFi_AP_Candidates.clearCache();
      ESPEasy::net::wifi::processScanDone();
      ESPEasy::net::wifi::WiFi_AP_Candidates.load_knownCredentials();

      if (!ESPEasy::net::wifi::WiFi_AP_Candidates.hasCandidates()) {
   # ifndef BUILD_MINIMAL_OTA
        addLog(LOG_LEVEL_INFO, F("Setup: Scan all channels"));
   # endif
        ESPEasy::net::wifi::WifiScan(false);
      }

      //    ESPEasy::net::wifi::setWifiMode(WIFI_OFF);
   */

  ESPEasy::net::wifi::initWiFi();
  return true;
}

bool NW001_data_struct_WiFi_STA::exit(EventStruct *event) {
  ESPEasy::net::wifi::exitWiFi();
  return true;
}

# ifdef ESP32

bool NW001_data_struct_WiFi_STA::handle_priority_route_changed()
{
  bool res{};

  if (WiFi.STA.isDefault()) {
    // Check to see if we may need to restore any cached DNS server
    for (size_t i = 0; i < NR_ELEMENTS(_dns_cache); ++i) {
      auto tmp = WiFi.STA.dnsIP(i);

      if ((_dns_cache[i] != INADDR_NONE) && (_dns_cache[i] != tmp)) {
        addLog(LOG_LEVEL_INFO, strformat(
                 F("NW001: Restore cached DNS server %d from %s to %s"),
                 i,
                 tmp.toString().c_str(),
                 _dns_cache[i].toString().c_str()
                 ));
        WiFi.STA.dnsIP(i, _dns_cache[i]);
        res = true;
      }
    }
  }
  return res;
}

# endif // ifdef ESP32


# ifdef ESP32

void NW001_data_struct_WiFi_STA::onEvent(arduino_event_id_t   event,
                                         arduino_event_info_t info)
{
  bool clear_wifi_event_sta_connected = false;

  switch (event)
  {
    case ARDUINO_EVENT_WIFI_OFF:
      clear_wifi_event_sta_connected = true;
      addLog(LOG_LEVEL_INFO, F("WIFI_OFF"));
      break;
    case ARDUINO_EVENT_WIFI_READY:
      //    clear_wifi_event_sta_connected = true;
      addLog(LOG_LEVEL_INFO, F("WIFI_READY"));
      break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
      addLog(LOG_LEVEL_INFO, F("SCAN_DONE"));
      break;
    case ARDUINO_EVENT_WIFI_STA_START:
      clear_wifi_event_sta_connected = true;
      _startStopStats.setOn();
      addLog(LOG_LEVEL_INFO, F("STA_START"));
      break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
      clear_wifi_event_sta_connected = true;
      _startStopStats.setOff();
      addLog(LOG_LEVEL_INFO, F("STA_STOP"));
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      memcpy(&_wifi_event_sta_connected, &info.wifi_sta_connected, sizeof(_wifi_event_sta_connected));
      _connectedStats.setOn();
      addLog(LOG_LEVEL_INFO, F("STA_CONNECTED"));
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      clear_wifi_event_sta_connected = true;
      _wifi_disconnect_reason        = static_cast<WiFiDisconnectReason>(info.wifi_sta_disconnected.reason);
      _connectedStats.setOff();
      addLog(LOG_LEVEL_INFO, F("STA_DISCONNECTED"));
      break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
      _wifi_event_sta_connected.authmode = info.wifi_sta_authmode_change.new_mode;
      addLog(LOG_LEVEL_INFO, F("STA_AUTHMODE_CHANGE"));
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:

      for (size_t i = 0; i < NR_ELEMENTS(_dns_cache); ++i) {
        auto tmp = WiFi.STA.dnsIP(i);

        if (tmp != INADDR_NONE) {
          _dns_cache[i] = tmp;
          addLog(LOG_LEVEL_INFO, strformat(F("DNS Cache %d set to %s"), i, tmp.toString(true).c_str()));
        }

      }
      _gotIPStats.setOn();
      addLog(LOG_LEVEL_INFO, F("STA_GOT_IP"));
      break;
#  if FEATURE_USE_IPV6
    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
      _gotIP6Stats.setOn();

      addLog(LOG_LEVEL_INFO, F("STA_GOT_IP6"));
      break;
#  endif // if FEATURE_USE_IPV6
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      _gotIPStats.setOff();
#  if FEATURE_USE_IPV6
      _gotIP6Stats.setOff();
#  endif

      addLog(LOG_LEVEL_INFO, F("STA_LOST_IP"));
      break;

    default:
      break;
  }

  if (clear_wifi_event_sta_connected) {
    memset(&_wifi_event_sta_connected, 0, sizeof(_wifi_event_sta_connected));
  }
}

# endif // ifdef ESP32

} // namespace wifi
} // namespace net
} // namespace ESPEasy

#endif // ifdef USES_NW001
