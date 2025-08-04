#include "../DataStructs/NWPluginData_base.h"


#include "../../../src/Globals/RuntimeData.h"
#include "../../../src/Globals/Settings.h"
#include "../../../src/Helpers/StringConverter.h"
#if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS
# include "../../../src/Helpers/_ESPEasy_key_value_store.h"
#endif
#ifdef ESP32
# include "../Globals/NetworkState.h"

# include <esp_netif.h>
# include <esp_netif_types.h>
#endif

namespace ESPEasy {
namespace net {

#ifdef ESP32
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
  ip_event_tx_rx_t *event = (ip_event_tx_rx_t *)event_data;
  const int key           = esp_netif_get_netif_impl_index(event->esp_netif);

  if (event->dir == ESP_NETIF_TX) {
    interfaceTrafficCount[key]._tx_count += event->len;
  } else if (event->dir == ESP_NETIF_RX) {
    interfaceTrafficCount[key]._rx_count += event->len;
    addLog(LOG_LEVEL_INFO, strformat(
             F("RX: %s key: %d len: %d total: %d"),
             esp_netif_get_desc(event->esp_netif),
             key,
             event->len,
             interfaceTrafficCount[key]._rx_count
             ));
  }
}

#endif // ifdef ESP32


NWPluginData_base::NWPluginData_base(
  nwpluginID_t nwpluginID, networkIndex_t networkIndex
#ifdef                ESP32
  , NetworkInterface *netif
#endif
  ) :
#if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS
  _kvs(nullptr),
#endif
  _nw_data_pluginID(nwpluginID),
  _networkIndex(networkIndex),
  _baseClassOnly(false)
#ifdef ESP32
  , _netif(netif)
#endif
{
  #ifdef ESP32
  const int key = esp_netif_get_netif_impl_index(_netif->netif());
  interfaceTrafficCount[key].clear();
  static bool registered_IP_EVENT_TX_RX = false;
  esp_netif_tx_rx_event_enable(_netif->netif());

  if (!registered_IP_EVENT_TX_RX) {
    esp_event_handler_register(IP_EVENT, IP_EVENT_TX_RX, &tx_rx_event_handler, NULL);
    registered_IP_EVENT_TX_RX = true;
  }
  esp_netif_tx_rx_event_enable(_netif->netif());
//  _netif->netif()->tx_rx_events_enabled = true;

  #endif // ifdef ESP32
#if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS

  if (_kvs == nullptr) {
    _kvs = new (std::nothrow) ESPEasy_key_value_store;
  }
#endif // if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS
}

NWPluginData_base::~NWPluginData_base()
{
#ifdef ESP32
  esp_netif_tx_rx_event_disable(_netif->netif());
  const int key = esp_netif_get_netif_impl_index(_netif->netif());
  auto it       = interfaceTrafficCount.find(key);

  if (it != interfaceTrafficCount.end()) { interfaceTrafficCount.erase(it); }
#endif // ifdef ESP32
#if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS

  if (_kvs) { delete _kvs; }
  _kvs = nullptr;
#endif // if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS
}

bool NWPluginData_base::plugin_write_base(EventStruct  *event,
                                          const String& string) { return false; }

#if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS

bool NWPluginData_base::init_KVS()
{
  if (!_KVS_initialized()) { return false; }

  //  _load();

  // TODO TD-er: load() can also return false when some other data used to be present. Have to think about how to handle this.
  return true;
}

#endif // if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS

LongTermTimer::Duration NWPluginData_base::getConnectedDuration_ms() {
  return getNWPluginData_static_runtime()._connectedStats.getLastOnDuration_ms();
}

#ifdef ESP32

bool NWPluginData_base::handle_priority_route_changed()
{
  bool res{};

  if ((_netif != nullptr) && _netif->isDefault()) {
    if (NWPlugin::forceDHCP_request(_netif)) { return true; }

    NWPluginData_static_runtime& cache = getNWPluginData_static_runtime();

    // Check to see if we may need to restore any cached DNS server
    for (size_t i = 0; i < NR_ELEMENTS(cache._dns_cache); ++i) {
      auto tmp = _netif->dnsIP(i);

      if ((cache._dns_cache[i] != INADDR_NONE) && (cache._dns_cache[i] != tmp)) {
        addLog(LOG_LEVEL_INFO, strformat(
                 F("%s: Restore cached DNS server %d from %s to %s"),
                 esp_netif_get_desc(_netif->netif()),
                 i,
                 tmp.toString().c_str(),
                 cache._dns_cache[i].toString().c_str()
                 ));

        _netif->dnsIP(i, cache._dns_cache[i]);
        res = true;
      }
    }
  }
  return res;
}

bool NWPluginData_base::getTrafficCount(uint64_t& tx, uint64_t& rx) const
{
  const int key = esp_netif_get_netif_impl_index(_netif->netif());
  auto it       = interfaceTrafficCount.find(key);

  if (it == interfaceTrafficCount.end()) { return false; }
  tx = it->second._tx_count;
  rx = it->second._rx_count;
  return true;
}

#endif // ifdef ESP32


#ifdef ESP32

void NWPluginData_base::_mark_got_IP(NWPluginData_static_runtime& cache)
{
  if (!cache._netif->isDefault()) {
    nonDefaultNetworkInterface_gotIP = true;
  }

  for (size_t i = 0; i < NR_ELEMENTS(cache._dns_cache); ++i) {
    auto tmp = cache._netif->dnsIP(i);

    if (tmp != INADDR_NONE) {
      cache._dns_cache[i] = tmp;
      addLog(LOG_LEVEL_INFO, strformat(
               F("%s: DNS Cache %d set to %s"),
               esp_netif_get_desc(cache._netif->netif()),
               i,
               tmp.toString(true).c_str()));
    }
  }
  addLog(LOG_LEVEL_INFO, strformat(
           F("%s: Got IP: %s"),
           esp_netif_get_desc(cache._netif->netif()),
           cache._netif->localIP().toString().c_str()
           ));
}

#else // ifdef ESP32

void NWPluginData_base::_mark_got_IP(NWPluginData_static_runtime& cache) { addLog(LOG_LEVEL_INFO, F("STA: Got IP")); }

#endif // ifdef ESP32

void NWPluginData_base::_mark_disconnected(const NWPluginData_static_runtime& cache)
{
  #ifdef ESP32
  addLog(LOG_LEVEL_INFO, strformat(
           F("%s: Disconnected. Connected for: "),
           esp_netif_get_desc(cache._netif->netif()),
           format_msec_duration_HMS(
             cache._connectedStats.getLastOnDuration_ms())));
#else // ifdef ESP32
  addLog(LOG_LEVEL_INFO, concat(
           F("STA: Disconnected. Connected for: "),
           format_msec_duration_HMS(cache._connectedStats.getLastOnDuration_ms())));
#endif // ifdef ESP32
}

#ifdef ESP32

bool NWPluginData_base::_restore_DNS_cache()
{
  bool res{};

  if ((_netif != nullptr) && _netif->isDefault()) {
    if (NWPlugin::forceDHCP_request(_netif)) { return true; }

    NWPluginData_static_runtime& cache = getNWPluginData_static_runtime();

    // Check to see if we may need to restore any cached DNS server
    for (size_t i = 0; i < NR_ELEMENTS(cache._dns_cache); ++i) {
      auto tmp = _netif->dnsIP(i);

      if ((cache._dns_cache[i] != INADDR_NONE) && (cache._dns_cache[i] != tmp)) {
        addLog(LOG_LEVEL_INFO, strformat(
                 F("NW%03%d: Restore cached DNS server %d from %s to %s"),
                 _nw_data_pluginID,
                 i,
                 tmp.toString().c_str(),
                 cache._dns_cache[i].toString().c_str()
                 ));

        _netif->dnsIP(i, cache._dns_cache[i]);
        res = true;
      }
    }
  }
  return res;
}

#endif // ifdef ESP32

#if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS

bool NWPluginData_base::_load()
{
  if (!_kvs) { return false; }
  return _kvs->load(
    SettingsType::Enum::NetworkInterfaceSettings_Type,
    _networkIndex,
    0,
    _nw_data_pluginID.value);
}

bool NWPluginData_base::_store()
{
  if (!_kvs) { return false; }
  return _kvs->store(
    SettingsType::Enum::NetworkInterfaceSettings_Type,
    _networkIndex,
    0,
    _nw_data_pluginID.value);
}

#endif // if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS

} // namespace net
} // namespace ESPEasy
