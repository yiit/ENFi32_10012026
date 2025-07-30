#include "../NWPluginStructs/NW001_data_struct_WiFi_STA.h"

#ifdef USES_NW001

# include "../../../src/Globals/Settings.h"

# include "../../../src/Helpers/ESPEasy_time_calc.h"
# include "../../../src/Helpers/LongTermOnOffTimer.h"
# include "../../../src/Helpers/StringConverter.h"

# include "../wifi/ESPEasyWifi.h"


namespace ESPEasy {
namespace net {
namespace wifi {

static LongTermOnOffTimer _startStopStats;
static LongTermOnOffTimer _connectedStats;
static LongTermOnOffTimer _gotIPStats;
static LongTermOnOffTimer _gotIP6Stats;


NW001_data_struct_WiFi_STA::NW001_data_struct_WiFi_STA(networkIndex_t networkIndex)
  : NWPluginData_base(nwpluginID_t(1), networkIndex)
{
  _connectedStats.clear();
  _gotIPStats.clear();
  _gotIP6Stats.clear();
  nw_event_id = Network.onEvent(NW001_data_struct_WiFi_STA::onEvent);
}

NW001_data_struct_WiFi_STA::~NW001_data_struct_WiFi_STA()
{
  if (nw_event_id != 0) {
    Network.removeEvent(nw_event_id);
  }
  nw_event_id = 0;
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

bool NW001_data_struct_WiFi_STA::exit(EventStruct *event) { return true; }

void NW001_data_struct_WiFi_STA::onEvent(arduino_event_id_t   event,
                                         arduino_event_info_t info)
{
  switch (event)
  {
    case ARDUINO_EVENT_WIFI_OFF:
      addLog(LOG_LEVEL_INFO, F("WIFI_OFF"));
      break;
    case ARDUINO_EVENT_WIFI_READY:
      addLog(LOG_LEVEL_INFO, F("WIFI_READY"));
      break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
      addLog(LOG_LEVEL_INFO, F("SCAN_DONE"));
      break;
    case ARDUINO_EVENT_WIFI_STA_START:
      _startStopStats.setOn();
      addLog(LOG_LEVEL_INFO, F("STA_START"));
      break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
      _startStopStats.setOff();
      addLog(LOG_LEVEL_INFO, F("STA_STOP"));
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      _connectedStats.setOn();
      addLog(LOG_LEVEL_INFO, F("STA_CONNECTED"));
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      _connectedStats.setOff();
      addLog(LOG_LEVEL_INFO, F("STA_DISCONNECTED"));
      break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
      addLog(LOG_LEVEL_INFO, F("STA_AUTHMODE_CHANGE"));
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      _gotIPStats.setOn();
      addLog(LOG_LEVEL_INFO, F("STA_GOT_IP"));
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
      _gotIP6Stats.setOn();
      addLog(LOG_LEVEL_INFO, F("STA_GOT_IP6"));
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      _gotIPStats.setOff();
      _gotIP6Stats.setOff();

      addLog(LOG_LEVEL_INFO, F("STA_LOST_IP"));
      break;

    default: break;
  }
}

} // namespace wifi
} // namespace net
} // namespace ESPEasy

#endif // ifdef USES_NW001
