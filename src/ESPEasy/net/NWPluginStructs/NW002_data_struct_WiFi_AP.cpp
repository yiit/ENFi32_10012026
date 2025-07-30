#include "../NWPluginStructs/NW002_data_struct_WiFi_AP.h"

#ifdef USES_NW002

# include "../../../src/Globals/Settings.h"

# include "../../../src/Helpers/ESPEasy_time_calc.h"
# include "../../../src/Helpers/LongTermOnOffTimer.h"
# include "../../../src/Helpers/StringConverter.h"

# include "../wifi/ESPEasyWifi.h"

namespace ESPEasy {
namespace net {
namespace wifi {

static LongTermOnOffTimer _startStopStats;


NW002_data_struct_WiFi_AP::NW002_data_struct_WiFi_AP(networkIndex_t networkIndex)
  : NWPluginData_base(nwpluginID_t(2), networkIndex)
{
  nw_event_id = Network.onEvent(NW002_data_struct_WiFi_AP::onEvent);
}

NW002_data_struct_WiFi_AP::~NW002_data_struct_WiFi_AP()
{
  if (nw_event_id != 0) {
    Network.removeEvent(nw_event_id);
  }
  nw_event_id = 0;
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
  WiFi.AP.end();
# endif
# ifdef ESP8266

  // WiFi.softAP
# endif // ifdef ESP8266

  return true;
}

void NW002_data_struct_WiFi_AP::onEvent(arduino_event_id_t   event,
                                        arduino_event_info_t info)
{
  switch (event)
  {
    case ARDUINO_EVENT_WIFI_AP_START:
      _startStopStats.setOn();
      addLog(LOG_LEVEL_INFO, F("AP_START"));
      break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
      _startStopStats.setOff();
      addLog(LOG_LEVEL_INFO, F("AP_STOP"));
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

} // namespace wifi
} // namespace net
} // namespace ESPEasy

#endif // ifdef USES_NW002
