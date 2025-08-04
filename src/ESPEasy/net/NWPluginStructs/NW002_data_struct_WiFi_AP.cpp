#include "../NWPluginStructs/NW002_data_struct_WiFi_AP.h"

#ifdef USES_NW002

# include "../../../src/Globals/Settings.h"

# include "../../../src/Helpers/ESPEasy_time_calc.h"
# include "../../../src/Helpers/LongTermOnOffTimer.h"
# include "../../../src/Helpers/StringConverter.h"

# include "../wifi/ESPEasyWifi.h"

# define NW_PLUGIN_ID  2
# ifdef ESP32
#  define NW_PLUGIN_INTERFACE   WiFi.AP
# endif

namespace ESPEasy {
namespace net {
namespace wifi {

# ifdef ESP32
static NWPluginData_static_runtime stats_and_cache(&NW_PLUGIN_INTERFACE);
# else
static NWPluginData_static_runtime stats_and_cache(false);
# endif // ifdef ESP32


NW002_data_struct_WiFi_AP::NW002_data_struct_WiFi_AP(networkIndex_t networkIndex)
  : NWPluginData_base(
    nwpluginID_t(NW_PLUGIN_ID)
    , networkIndex
# ifdef ESP32
    , &NW_PLUGIN_INTERFACE
# endif
    )
{
  stats_and_cache.clear(networkIndex);
# ifdef ESP32
  nw_event_id = Network.onEvent(NW002_data_struct_WiFi_AP::onEvent);
# endif
}

NW002_data_struct_WiFi_AP::~NW002_data_struct_WiFi_AP()
{
# ifdef ESP32

  if (nw_event_id != 0) {
    Network.removeEvent(nw_event_id);
  }
  nw_event_id = 0;
# endif // ifdef ESP32
  stats_and_cache.clear();
}

void NW002_data_struct_WiFi_AP::webform_load(EventStruct *event) {}

void NW002_data_struct_WiFi_AP::webform_save(EventStruct *event) {}

bool NW002_data_struct_WiFi_AP::webform_getPort(String& str)     { return true; }

bool NW002_data_struct_WiFi_AP::init(EventStruct *event)
{
  ESPEasy::net::wifi::setAPinternal(true);
  return true;
}

bool NW002_data_struct_WiFi_AP::exit(EventStruct *event)
{
# ifdef ESP32
  NW_PLUGIN_INTERFACE.end();
# endif
# ifdef ESP8266

  // WiFi.softAP
# endif // ifdef ESP8266

  return true;
}

NWPluginData_static_runtime& NW002_data_struct_WiFi_AP::getNWPluginData_static_runtime() { return stats_and_cache; }


# ifdef ESP32

bool NW002_data_struct_WiFi_AP::handle_priority_route_changed()
{
  bool res{};

  // NW_PLUGIN_INTERFACE.enableNAPT(false);

  if (!NW_PLUGIN_INTERFACE.isDefault()) {
    // FIXME TD-er: May need to stop/start NAPT on WiFi.AP
    // NW_PLUGIN_INTERFACE.enableNAPT(true);
  }

  return res;
}

void NW002_data_struct_WiFi_AP::onEvent(arduino_event_id_t   event,
                                        arduino_event_info_t info)
{
  switch (event)
  {
    case ARDUINO_EVENT_WIFI_AP_START:
      stats_and_cache.mark_start();
      break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
      stats_and_cache.mark_stop();
      break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      addLog(LOG_LEVEL_INFO, F("AP_STACONNECTED"));
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      addLog(LOG_LEVEL_INFO, F("AP_STADISCONNECTED"));
      break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
      addLog(LOG_LEVEL_INFO, F("AP_STAIPASSIGNED"));
      break;
    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
      addLog(LOG_LEVEL_INFO, F("AP_PROBEREQRECVED"));
      break;
    case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
      addLog(LOG_LEVEL_INFO, F("AP_GOT_IP6"));
      break;

    default: break;
  }
}

# endif // ifdef ESP32

} // namespace wifi
} // namespace net
} // namespace ESPEasy

#endif // ifdef USES_NW002
