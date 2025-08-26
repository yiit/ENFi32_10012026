#include "../DataStructs/NWPluginData_static_runtime.h"

#include "../../../src/Globals/EventQueue.h"
#include "../../../src/Globals/Settings.h"

#include "../../../src/Helpers/NetworkStatusLED.h"
#include "../../../src/Helpers/StringConverter.h"

namespace ESPEasy {
namespace net {

#define CONNECTION_CONSIDERED_STABLE_MSEC    60000
#define CONNECT_TIMEOUT_MAX                  10000 // in milliSeconds

void NWPluginData_static_runtime::clear(networkIndex_t networkIndex)
{
  _connectedStats.clear();
  _gotIPStats.clear();
#if FEATURE_USE_IPV6
  _gotIP6Stats.clear();
#endif
  _networkIndex = networkIndex;
#ifdef ESP32
  _route_prio = Settings.getRoutePrio_for_network(_networkIndex);
  if (_netif) {
    if (_eventInterfaceName.length() == 0 && _netif) {
      _eventInterfaceName = _netif->desc();
      _eventInterfaceName.toUpperCase();
    }
  }
#endif

  _connectionFailures = 0;

  // FIXME TD-er: Should also clear dns cache?
}

void NWPluginData_static_runtime::processEvent_and_clear()
{
  processEvents();
  clear();
}


bool NWPluginData_static_runtime::operational() const
{
  return started() && connected() && hasIP()
   && Settings.getNetworkEnabled(_networkIndex);
   // FIXME TD-er: WiFi STA keeps reporting it is 
   // connected and has IP even after call to networkdisable,1
}

void NWPluginData_static_runtime::processEvents()
{
  if (_establishConnectStats.changedSinceLastCheck_and_clear())
  {
    if (_establishConnectStats.isOn()) {
      log_connected();
//    _establishConnectStats.resetCount();
    } else {
      log_disconnected();
    }
  }


  _operationalStats.set(operational());
  if (_operationalStats.changedSinceLastCheck_and_clear()) {
    // Send out event
    if (Settings.UseRules && _eventInterfaceName.length())
    {
      if (_operationalStats.isOn()) {
        eventQueue.add(concat(
          _eventInterfaceName, 
          F("#Connected")));
      } else {
        eventQueue.add(concat(
          _eventInterfaceName, 
          F("#Disconnected")));
      }
    }
    statusLED(true);
  }
}

String NWPluginData_static_runtime::statusToString() const
{
  String log;

  if (connected()) {
    log += F("Conn. ");
  }

  if (hasIP()) {
    log += F("IP ");
  }

  if (operational()) {
    log += F("Init");
  }

  if (log.isEmpty()) { log = F("DISCONNECTED"); }
  return log;
}

bool NWPluginData_static_runtime::stableConnection() const
{
  return _connectedStats.isOn() && _connectedStats.getLastOnDuration_ms() > CONNECTION_CONSIDERED_STABLE_MSEC;
}

uint32_t NWPluginData_static_runtime::getSuggestedTimeout(
  int      index,
  uint32_t minimum_timeout) const
{
  auto it = _connectDurations.find(index);

  if (it == _connectDurations.end()) {
    // Store the suggested timeout as negative value,
    // to make clear it was last suggested, but not yet confirmed.
    _connectDurations[index] = -3 * minimum_timeout;
    return 3 * minimum_timeout;
  }
  const uint32_t returnvalue = constrain(std::abs(3 * it->second), minimum_timeout, CONNECT_TIMEOUT_MAX);

  if (it->second < 0) {
    _connectDurations[index] = -1 * returnvalue;
  } else {
    _connectDurations[index] = returnvalue;
  }
  return returnvalue;
}

void NWPluginData_static_runtime::markConnectionSuccess(
  int      index,
  uint32_t duration_ms) const
{
  auto it = _connectDurations.find(index);

  if ((it == _connectDurations.end()) || (it->second < 0)) { _connectDurations[index] = duration_ms; }

  // Apply some slight filtering so we won't get into trouble when a connection suddenly was faster.
  _connectDurations[index] += 2 * duration_ms;
  _connectDurations[index] /= 3;

  if (_connectionFailures > 0) {
    --_connectionFailures;
  }
}

void NWPluginData_static_runtime::markPublishSuccess() const
{
  if (_connectionFailures > 0) {
    --_connectionFailures;
  }
}

void NWPluginData_static_runtime::markConnectionFailed(int index) const
{
  ++_connectionFailures;
}

} // namespace net
} // namespace ESPEasy
