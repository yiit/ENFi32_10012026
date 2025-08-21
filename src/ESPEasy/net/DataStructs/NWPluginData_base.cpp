#include "../DataStructs/NWPluginData_base.h"

#include "../../../src/DataStructs/ESPEasy_EventStruct.h"
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
#endif // ifdef ESP32

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
  if (event_data == nullptr) { return; }
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
#if FEATURE_PLUGIN_STATS
  _plugin_stats_array(nullptr),
#endif // if FEATURE_PLUGIN_STATS

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

  if (_netif) {
    const int key = _netif->impl_index();
    interfaceTrafficCount[key].clear();

    if (_netif->netif()) {
      static bool registered_IP_EVENT_TX_RX = false;
      esp_netif_tx_rx_event_enable(_netif->netif());

      if (!registered_IP_EVENT_TX_RX) {
        esp_event_handler_register(IP_EVENT, IP_EVENT_TX_RX, &tx_rx_event_handler, NULL);
        registered_IP_EVENT_TX_RX = true;
      }
      esp_netif_tx_rx_event_enable(_netif->netif());
    }

    //  _netif->netif()->tx_rx_events_enabled = true;
  }
  #endif // ifdef ESP32
#if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS

  if (_kvs == nullptr) {
    _kvs = new (std::nothrow) ESPEasy_key_value_store;
  }
#endif // if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS
}

NWPluginData_base::~NWPluginData_base()
{
#if FEATURE_PLUGIN_STATS
  delete _plugin_stats_array;
  _plugin_stats_array = nullptr;
#endif // if FEATURE_PLUGIN_STATS

#ifdef ESP32

  if (_netif) {
    if (_netif->netif()) {
      esp_netif_tx_rx_event_disable(_netif->netif());
    }
    const int key = _netif->impl_index();
    auto it       = interfaceTrafficCount.find(key);

    if (it != interfaceTrafficCount.end()) { interfaceTrafficCount.erase(it); }
  }
#endif // ifdef ESP32
#if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS

  if (_kvs) { delete _kvs; }
  _kvs = nullptr;
#endif // if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS
}

bool NWPluginData_base::hasPluginStats() const {
#if FEATURE_PLUGIN_STATS

  if (_plugin_stats_array != nullptr) {
    return _plugin_stats_array->hasStats();
  }
#endif // if FEATURE_PLUGIN_STATS
  return false;
}

bool NWPluginData_base::hasPeaks() const {
#if FEATURE_PLUGIN_STATS

  if (_plugin_stats_array != nullptr) {
    return _plugin_stats_array->hasPeaks();
  }
#endif // if FEATURE_PLUGIN_STATS
  return false;
}

size_t NWPluginData_base::nrSamplesPresent() const {
#if FEATURE_PLUGIN_STATS

  if (_plugin_stats_array != nullptr) {
    return _plugin_stats_array->nrSamplesPresent();
  }
#endif // if FEATURE_PLUGIN_STATS
  return 0;
}

#if FEATURE_PLUGIN_STATS

void NWPluginData_base::initPluginStats(
  networkStatsVarIndex_t      networkStatsVarIndex,
  const String              & label,
  uint8_t                     nrDecimals,
  float                       errorValue,
  const PluginStats_Config_t& displayConfig)
{
  if (networkStatsVarIndex < INVALID_NETWORK_STATS_VAR_INDEX) {
    if (_plugin_stats_array == nullptr) {
      constexpr unsigned size = sizeof(PluginStats_array);
      void *ptr               = special_calloc(1, size);

      if (ptr != nullptr) {
        _plugin_stats_array = new (ptr) PluginStats_array();
      }
    }

    if (_plugin_stats_array != nullptr) {
      _plugin_stats_array->initPluginStats(
        networkStatsVarIndex,
        label,
        nrDecimals,
        errorValue,
        displayConfig);
    }
  }
}

# ifdef ESP32

void NWPluginData_base::initPluginStats_trafficCount(networkStatsVarIndex_t networkStatsVarIndex, bool isTX)
{
  PluginStats_Config_t displayConfig;

  displayConfig.setAxisPosition(PluginStats_Config_t::AxisPosition::Left);
  displayConfig.setEnabled(true);
  displayConfig.setAxisIndex(3); // Set to a fixed index so RX/TX are on the same axis index
  initPluginStats(
    networkStatsVarIndex,
    isTX ? F("TX") : F("RX"),
    0,
    NAN,
    displayConfig);
}

# endif // ifdef ESP32

bool NWPluginData_base::initPluginStats()
{
# ifdef ESP32
  initPluginStats_trafficCount(0, true);  // TX
  initPluginStats_trafficCount(1, false); // RX
  return true;
# else // ifdef ESP32
  return false;
# endif // ifdef ESP32
}

void NWPluginData_base::clearPluginStats(networkStatsVarIndex_t networkStatsVarIndex)
{
  if ((networkStatsVarIndex < INVALID_NETWORK_STATS_VAR_INDEX) && _plugin_stats_array) {
    _plugin_stats_array->clearPluginStats(networkStatsVarIndex);

    if (!_plugin_stats_array->hasStats()) {
      delete _plugin_stats_array;
      _plugin_stats_array = nullptr;
    }
  }
}

void NWPluginData_base::processTimeSet(const double& time_offset)
{
  if (_plugin_stats_array != nullptr) {
    _plugin_stats_array->processTimeSet(time_offset);
  }
}

#endif // if FEATURE_PLUGIN_STATS

bool NWPluginData_base::pushStatsValues(EventStruct *event,
                                        size_t       valueCount,
                                        bool         trackPeaks,
                                        bool         onlyUpdateTimestampWhenSame)
{
#if FEATURE_PLUGIN_STATS

  if (_plugin_stats_array != nullptr) {
# ifdef ESP32

    // Include traffic
    uint64_t tx{};
    uint64_t rx{};

    if (getTrafficCount(tx, rx)) {
      // Only set value when _prevRX/TX was set to make sure there isn't an enormous spike
      event->ParfN[valueCount++] = _prevTX == 0 ? 0 : tx - _prevTX;
      event->ParfN[valueCount++] = _prevRX == 0 ? 0 : rx - _prevRX;
      _prevRX                    = rx;
      _prevTX                    = tx;
    } else {
      event->ParfN[valueCount++] = NAN;
      event->ParfN[valueCount++] = NAN;
      _prevTX                    = 0;
      _prevRX                    = 0;
    }
# endif // ifdef ESP32

    if (valueCount) {
      return _plugin_stats_array->pushStatsValues(event, valueCount, trackPeaks, onlyUpdateTimestampWhenSame);
    }
  }
#endif // if FEATURE_PLUGIN_STATS
  return false;
}

bool NWPluginData_base::plugin_write_base(EventStruct  *event,
                                          const String& string)
{
#if FEATURE_PLUGIN_STATS

  if (_plugin_stats_array != nullptr) {
    return _plugin_stats_array->plugin_write_base(event, string);
  }
#endif // if FEATURE_PLUGIN_STATS

  return false;
}

#if FEATURE_PLUGIN_STATS

bool NWPluginData_base::record_stats()
{
  if (_plugin_stats_array != nullptr) {
    # ifdef ESP32

    EventStruct tmpEvent;
    size_t valueCount{};
    bool   trackPeaks                  = true;
    bool   onlyUpdateTimestampWhenSame = true;
    return pushStatsValues(&tmpEvent, valueCount, trackPeaks, onlyUpdateTimestampWhenSame);
# endif // ifdef ESP32
  }
  return false;
}

bool NWPluginData_base::webformLoad_show_stats(EventStruct *event) const
{
  if (_plugin_stats_array != nullptr) {
    return _plugin_stats_array->webformLoad_show_stats(event);
  }
  return false;
}

# if FEATURE_CHART_JS

void NWPluginData_base::plot_ChartJS(bool onlyJSON) const
{
  if (_plugin_stats_array != nullptr) {
    _plugin_stats_array->plot_ChartJS(onlyJSON);
  }
}

void NWPluginData_base::plot_ChartJS_scatter(
  networkStatsVarIndex_t        values_X_axis_index,
  networkStatsVarIndex_t        values_Y_axis_index,
  const __FlashStringHelper    *id,
  const ChartJS_title         & chartTitle,
  const ChartJS_dataset_config& datasetConfig,
  int                           width,
  int                           height,
  bool                          showAverage,
  const String                & options,
  bool                          onlyJSON) const
{
  if (_plugin_stats_array != nullptr) {
    // TODO TD-er: Show TX-power vs. RSSI as scatter plot
    _plugin_stats_array->plot_ChartJS_scatter(
      values_X_axis_index,
      values_Y_axis_index,
      id,
      chartTitle,
      datasetConfig,
      width,
      height,
      showAverage,
      options,
      onlyJSON);
  }
}

# endif // if FEATURE_CHART_JS
#endif // if FEATURE_PLUGIN_STATS


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
  auto data = getNWPluginData_static_runtime();

  if (data) {
    return data->_connectedStats.getLastOnDuration_ms();
  }
  return 0;
}

bool NWPluginData_base::handle_nwplugin_write(EventStruct *event, String& str)
{
  return false;
}

#ifdef ESP32

bool NWPluginData_base::handle_priority_route_changed()
{
  bool res{};

  if ((_netif != nullptr) && _netif->isDefault()) {
    if (NWPlugin::forceDHCP_request(_netif)) { return true; }

    auto cache = getNWPluginData_static_runtime();

    if (!cache) { return res; }

    // Check to see if we may need to restore any cached DNS server
    for (size_t i = 0; i < NR_ELEMENTS(cache->_dns_cache); ++i) {
      auto tmp = _netif->dnsIP(i);

      if ((cache->_dns_cache[i] != INADDR_NONE) && (cache->_dns_cache[i] != tmp)) {
        addLog(LOG_LEVEL_INFO, strformat(
                 F("%s: Restore cached DNS server %d from %s to %s"),
                 _netif->desc(),
                 i,
                 tmp.toString().c_str(),
                 cache->_dns_cache[i].toString().c_str()
                 ));

        _netif->dnsIP(i, cache->_dns_cache[i]);
        res = true;
      }
    }
  }
  return res;
}

bool NWPluginData_base::getTrafficCount(uint64_t& tx, uint64_t& rx) const
{
  if (!_netif) { return false; }
  const int key = _netif->impl_index();
  auto it       = interfaceTrafficCount.find(key);

  if (it == interfaceTrafficCount.end()) { return false; }
  tx = it->second._tx_count;
  rx = it->second._rx_count;
  return true;
}

#endif // ifdef ESP32

#if FEATURE_PLUGIN_STATS

PluginStats * NWPluginData_base::getPluginStats(networkStatsVarIndex_t networkStatsVarIndex) const
{
  if (_plugin_stats_array != nullptr) {
    return _plugin_stats_array->getPluginStats(networkStatsVarIndex);
  }
  return nullptr;
}

PluginStats * NWPluginData_base::getPluginStats(networkStatsVarIndex_t networkStatsVarIndex)
{
  if (_plugin_stats_array != nullptr) {
    return _plugin_stats_array->getPluginStats(networkStatsVarIndex);
  }
  return nullptr;
}

#endif // if FEATURE_PLUGIN_STATS

#ifdef ESP32

bool NWPluginData_base::_restore_DNS_cache()
{
  bool res{};

  if ((_netif != nullptr) && _netif->isDefault()) {
    if (NWPlugin::forceDHCP_request(_netif)) { return true; }

    auto cache = getNWPluginData_static_runtime();

    if (!cache) { return res; }

    // Check to see if we may need to restore any cached DNS server
    for (size_t i = 0; i < NR_ELEMENTS(cache->_dns_cache); ++i) {
      auto tmp = _netif->dnsIP(i);

      if ((cache->_dns_cache[i] != INADDR_NONE) && (cache->_dns_cache[i] != tmp)) {
        addLog(LOG_LEVEL_INFO, strformat(
                 F("NW%03%d: Restore cached DNS server %d from %s to %s"),
                 _nw_data_pluginID,
                 i,
                 tmp.toString().c_str(),
                 cache->_dns_cache[i].toString().c_str()
                 ));

        _netif->dnsIP(i, cache->_dns_cache[i]);
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
