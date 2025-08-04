#include "../DataStructs/NWPluginData_static_runtime.h"

#ifdef ESP8266

# include "../../../src/Helpers/StringConverter.h"

namespace ESPEasy {
namespace net {

bool NWPluginData_static_runtime::connected() const
{
  if (_isSTA) { return WiFi.status() == WL_CONNECTED; }
  return false;
}

bool NWPluginData_static_runtime::isDefaultRoute() const
{
  // FIXME TD-er: Should I just return connected() here?
  return _isSTA;
}

bool NWPluginData_static_runtime::hasIP() const
{
  return _gotIPStats.isOn();
}

void NWPluginData_static_runtime::mark_start()
{
  _startStopStats.setOn();
  addLog(LOG_LEVEL_INFO, _isSTA ? F("STA: Started") : F("AP: Started"));
}

void NWPluginData_static_runtime::mark_stop()
{
  _startStopStats.setOff();
  addLog(LOG_LEVEL_INFO, _isSTA ? F("STA: Stopped") : F("AP: Stopped"));
}

void NWPluginData_static_runtime::mark_got_IP()
{
  // Set OnOffTimer to off so we can also count how often we het new IP
  _gotIPStats.setOff();

  _gotIPStats.setOn();

  addLog(LOG_LEVEL_INFO, _isSTA ? F("STA: Got IP") : F("AP: Got IP"));
}

void NWPluginData_static_runtime::mark_lost_IP()
{
  _gotIPStats.setOff();
  addLog(LOG_LEVEL_INFO, _isSTA ? F("STA: Lost IP") : F("AP: Lost IP"));
}

void NWPluginData_static_runtime::mark_connected()
{
  _connectedStats.setOn();
  addLog(LOG_LEVEL_INFO, F("STA: Connected"));
}

void NWPluginData_static_runtime::mark_disconnected()
{
  _connectedStats.setOff();

  if (_isSTA) {
    addLog(LOG_LEVEL_INFO, concat(
             F("STA: Disconnected. Connected for: "),
             format_msec_duration_HMS(_connectedStats.getLastOnDuration_ms())));
  }
}

} // namespace net
} // namespace ESPEasy

#endif // ifdef ESP8266
