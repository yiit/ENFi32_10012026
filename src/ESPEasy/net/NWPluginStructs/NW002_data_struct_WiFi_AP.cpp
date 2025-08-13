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

# define NW002_STATION_COUNT_STATS_INDEX       0

namespace ESPEasy {
namespace net {
namespace wifi {

# ifdef ESP32
static NWPluginData_static_runtime stats_and_cache(&NW_PLUGIN_INTERFACE);
# else
static NWPluginData_static_runtime stats_and_cache(false);
# endif // ifdef ESP32

# ifdef ESP32
static bool nw002_enable_NAPT{};

static bool NW002_update_NAPT() {
  NW_PLUGIN_INTERFACE.enableNAPT(false);

  if (nw002_enable_NAPT && NW_PLUGIN_INTERFACE.stationCount()) {
    NW_PLUGIN_INTERFACE.enableNAPT(true);
    return true;
  }

  return false;
}

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
  nw002_enable_NAPT = Settings.WiFi_AP_enable_NAPT();
  nw_event_id       = Network.onEvent(NW002_data_struct_WiFi_AP::onEvent);
# endif // ifdef ESP32
}

NW002_data_struct_WiFi_AP::~NW002_data_struct_WiFi_AP()
{
# ifdef ESP32
  nw002_enable_NAPT = false;

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
# ifdef ESP32
  nw002_enable_NAPT = Settings.WiFi_AP_enable_NAPT();
# endif
  ESPEasy::net::wifi::setAPinternal(true);
# ifdef ESP32
  NW002_update_NAPT();
# endif
  return true;
}

bool NW002_data_struct_WiFi_AP::exit(EventStruct *event)
{
# ifdef ESP32
  NW_PLUGIN_INTERFACE.enableNAPT(false);
  NW_PLUGIN_INTERFACE.end();
# endif // ifdef ESP32
# ifdef ESP8266
  WiFi.softAPdisconnect();
# endif // ifdef ESP8266

  return true;
}

NWPluginData_static_runtime& NW002_data_struct_WiFi_AP::getNWPluginData_static_runtime() { return stats_and_cache; }


# ifdef ESP32

bool NW002_data_struct_WiFi_AP::handle_priority_route_changed() { return NW002_update_NAPT(); }

# endif // ifdef ESP32


# if FEATURE_PLUGIN_STATS

bool NW002_data_struct_WiFi_AP::initPluginStats()
{
  networkStatsVarIndex_t networkStatsVarIndex{};
  PluginStats_Config_t   displayConfig;

  displayConfig.setAxisPosition(PluginStats_Config_t::AxisPosition::Right);
  displayConfig.setEnabled(true);

  displayConfig.setAxisIndex(networkStatsVarIndex);
  NWPluginData_base::initPluginStats(
    networkStatsVarIndex,
    F("Station Count"),
    1,
    NAN,
    displayConfig);
#  ifdef ESP32
  initPluginStats_trafficCount(++networkStatsVarIndex, true);  // TX
  initPluginStats_trafficCount(++networkStatsVarIndex, false); // RX
#  endif // ifdef ESP32
  return true;
}

bool NW002_data_struct_WiFi_AP::record_stats()
{
  if (_plugin_stats_array != nullptr) {
    EventStruct tmpEvent;
    size_t valueCount{};
    tmpEvent.ParfN[valueCount++] =
    #  ifdef ESP32
      NW_PLUGIN_INTERFACE.stationCount();
    #  else
      WiFi.softAPgetStationNum();
    #  endif // ifdef ESP32

    bool trackPeaks                  = true;
    bool onlyUpdateTimestampWhenSame = true;
    return pushStatsValues(&tmpEvent, valueCount, trackPeaks, onlyUpdateTimestampWhenSame);
  }
  return false;
}

bool NW002_data_struct_WiFi_AP::webformLoad_show_stats(struct EventStruct *event) const
{
  if (_plugin_stats_array != nullptr) {
    return _plugin_stats_array->webformLoad_show_stats(event);
  }
  return false;
}

# endif // if FEATURE_PLUGIN_STATS

# ifdef ESP32

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
      NW002_update_NAPT();
      addLog(LOG_LEVEL_INFO, F("AP_STACONNECTED"));
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      NW002_update_NAPT();
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
