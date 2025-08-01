#include "../NWPluginStructs/NW001_data_struct_WiFi_STA.h"

#ifdef USES_NW001

# include "../../../src/Globals/Settings.h"

# include "../../../src/Helpers/ESPEasy_time_calc.h"
# include "../../../src/Helpers/LongTermOnOffTimer.h"
# include "../../../src/Helpers/StringConverter.h"

# include "../wifi/ESPEasyWifi.h"

# define NW_PLUGIN_ID  1
# define NW_PLUGIN_INTERFACE   WiFi.STA

namespace ESPEasy {
namespace net {
namespace wifi {


NW001_data_struct_WiFi_STA::NW001_data_struct_WiFi_STA(networkIndex_t networkIndex)
  : NWPluginData_base(nwpluginID_t(NW_PLUGIN_ID), networkIndex)
{}

NW001_data_struct_WiFi_STA::~NW001_data_struct_WiFi_STA()
{}

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
  if (NW_PLUGIN_INTERFACE.isDefault()) {
    if (NWPlugin::forceDHCP_request(&NW_PLUGIN_INTERFACE)) { return true; }
    return _WiFiEventHandler.restore_dns_from_cache();
  }
  return false;
}

# endif // ifdef ESP32


} // namespace wifi
} // namespace net
} // namespace ESPEasy

#endif // ifdef USES_NW001
