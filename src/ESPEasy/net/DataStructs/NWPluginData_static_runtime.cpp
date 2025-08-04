#include "../DataStructs/NWPluginData_static_runtime.h"


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

  // FIXME TD-er: Should also clear dns cache?
}

bool NWPluginData_static_runtime::operational() const
{
  return _startStopStats.isOn() && connected() && hasIP();
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

void NWPluginData_static_runtime::setConnectionDuration(
  int      index,
  uint32_t duration_ms) const
{
  auto it = _connectDurations.find(index);

  if ((it == _connectDurations.end()) || (it->second < 0)) { _connectDurations[index] = duration_ms; }

  // Apply some slight filtering so we won't get into trouble when a connection suddenly was faster.
  _connectDurations[index] += 2 * duration_ms;
  _connectDurations[index] /= 3;
}

} // namespace net
} // namespace ESPEasy
