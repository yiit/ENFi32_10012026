#include "../DataStructs/NWPluginData_static_runtime.h"

#include "../../../src/Globals/EventQueue.h"
#include "../../../src/Globals/Settings.h"

#include "../../../src/Helpers/NetworkStatusLED.h"
#include "../../../src/Helpers/StringConverter.h"

namespace ESPEasy {
namespace net {

#define CONNECTION_CONSIDERED_STABLE_MSEC    60000
#define CONNECT_TIMEOUT_MAX                  10000 // in milliSeconds

#if FEATURE_NETWORK_TRAFFIC_COUNT
struct TX_RX_traffic_count {

  void clear() { _tx_count = 0; _rx_count = 0; }

  uint64_t _tx_count{};
  uint64_t _rx_count{};

};

typedef std::map<int, TX_RX_traffic_count> InterfaceTrafficCount_t;

static InterfaceTrafficCount_t interfaceTrafficCount;

static void tx_rx_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
  if (event_data == nullptr) { return; }
  ip_event_tx_rx_t *event = (ip_event_tx_rx_t *)event_data;
  const int key           = esp_netif_get_netif_impl_index(event->esp_netif);

  if (event->dir == ESP_NETIF_TX) {
    interfaceTrafficCount[key]._tx_count += event->len;
  } else if (event->dir == ESP_NETIF_RX) {
    interfaceTrafficCount[key]._rx_count += event->len;

    /*
        addLog(LOG_LEVEL_INFO, strformat(
                 F("RX: %s key: %d len: %d total: %d"),
                 esp_netif_get_desc(event->esp_netif),
                 key,
                 event->len,
                 interfaceTrafficCount[key]._rx_count
                 ));
     */
  }
}

void NWPluginData_static_runtime::enable_txrx_events()
{
  if (_netif) {
    const int key = _netif->impl_index();
    interfaceTrafficCount[key].clear();

    if (_netif->netif()) {
      static bool registered_IP_EVENT_TX_RX = false;
      const int   key                       = esp_netif_get_netif_impl_index(_netif->netif());
      interfaceTrafficCount[key].clear();
      esp_netif_tx_rx_event_enable(_netif->netif());

      if (!registered_IP_EVENT_TX_RX) {
        esp_event_handler_instance_register(IP_EVENT, IP_EVENT_TX_RX, &tx_rx_event_handler, NULL, &_handler_inst);
        registered_IP_EVENT_TX_RX = true;
      }
      esp_netif_tx_rx_event_enable(_netif->netif());
    }

    //  _netif->netif()->tx_rx_events_enabled = true;
  }
}

bool NWPluginData_static_runtime::getTrafficCount(uint64_t& tx, uint64_t& rx) const
{
  const int key = _netif->impl_index();
  auto it       = interfaceTrafficCount.find(key);

  if (it == interfaceTrafficCount.end()) { return false; }
  tx = it->second._tx_count;
  rx = it->second._rx_count;
  return true;
}

#endif // if FEATURE_NETWORK_TRAFFIC_COUNT

void NWPluginData_static_runtime::clear(networkIndex_t networkIndex)
{
  _connectedStats.clear();
  _gotIPStats.clear();
#if FEATURE_USE_IPV6
  _gotIP6Stats.clear();
#endif
  _operationalStats.clear();
#if FEATURE_NETWORK_TRAFFIC_COUNT
  if (_netif) {
    const int key = _netif->impl_index();
    interfaceTrafficCount[key].clear();
  }
#endif

  _networkIndex = networkIndex;
#ifdef ESP32
  _route_prio = Settings.getRoutePrio_for_network(_networkIndex);

  if (_netif) {
    if ((_eventInterfaceName.length() == 0) && _netif) {
      _eventInterfaceName = _netif->desc();
      _eventInterfaceName.toUpperCase();
    }
  }
#endif // ifdef ESP32

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
  return connected() && hasIP()
         && Settings.getNetworkEnabled(_networkIndex);

  // FIXME TD-er: WiFi STA keeps reporting it is
  // connected and has IP even after call to networkdisable,1
}

void NWPluginData_static_runtime::processEvents()
{
  // TD-er: Just set these just to be sure we didn't miss any events.
  _connectedStats.set(connected());
  _gotIPStats.set(hasIP());
#if FEATURE_USE_IPV6
  _gotIP6Stats.set(hasIPv6());
#endif
  const bool connected_changed        = _connectedStats.changedSinceLastCheck_and_clear();
  const bool establishConnect_changed = _establishConnectStats.changedSinceLastCheck_and_clear();
  const bool gotIP_changed            = _gotIPStats.changedSinceLastCheck_and_clear();

  if (connected_changed || establishConnect_changed)
  {
    if (_connectedStats.isOn()) {
      log_connected();

      //    _establishConnectStats.resetCount();
    } else if (_connectedStats.isOff()) {
      log_disconnected();
    }
  }

//  if (connected_changed || establishConnect_changed || gotIP_changed) {
//    _operationalStats.forceSet(operational());
//  } else {
    _operationalStats.set(operational());
//  }

  if (_operationalStats.changedSinceLastCheck_and_clear()) {
#if FEATURE_NETWORK_TRAFFIC_COUNT

    if (_operationalStats.isOn()) {
      enable_txrx_events();
    }
#endif // if FEATURE_NETWORK_TRAFFIC_COUNT

    // Send out event
    if (Settings.UseRules && _eventInterfaceName.length())
    {
      if (_operationalStats.isOn()) {
        eventQueue.add(concat(
                         _eventInterfaceName,
                         F("#Connected")));
      } else if (_operationalStats.isOff()) {
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
