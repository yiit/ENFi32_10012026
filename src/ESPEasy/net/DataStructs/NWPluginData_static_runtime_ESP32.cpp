#include "../DataStructs/NWPluginData_static_runtime.h"

#ifdef ESP32

# include "../../../src/Helpers/StringConverter.h"

# include "../Globals/NetworkState.h"

# include <esp_netif.h>
# include <esp_netif_types.h>


namespace ESPEasy {
namespace net {

bool NWPluginData_static_runtime::connected() const
{
  if (!_netif) { return false; }
  return _netif->connected();
}

bool NWPluginData_static_runtime::isDefaultRoute() const
{
  if (!_netif) { return false; }
  return _netif->isDefault();
}

bool NWPluginData_static_runtime::hasIP() const
{
  if (!_netif) { return false; }
  return _netif->hasIP();
}

# if FEATURE_USE_IPV6

bool NWPluginData_static_runtime::hasIPv6() const
{
  if (!_netif) { return false; }
  return _netif->hasGlobalIPv6() || _netif->hasLinkLocalIPv6();
}

# endif // if FEATURE_USE_IPV6

void NWPluginData_static_runtime::mark_start()
{
  _startStopStats.setOn();

  if (!_netif) { return; }
# if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0)

  if (_route_prio > 0) {
    _netif->setRoutePrio(_route_prio);
  }
# endif // if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0)

  if (loglevelActiveFor(LOG_LEVEL_INFO)) {
    addLog(LOG_LEVEL_INFO, strformat(
             F("%s: Started"),
             esp_netif_get_desc(_netif->netif())));
  }
}

void NWPluginData_static_runtime::mark_stop()
{
  _startStopStats.setOff();

  if (_netif && loglevelActiveFor(LOG_LEVEL_INFO)) {
    addLog(LOG_LEVEL_INFO, strformat(
             F("%s: Stopped"),
             esp_netif_get_desc(_netif->netif())));
  }
}

void NWPluginData_static_runtime::mark_got_IP()
{
  // Set OnOffTimer to off so we can also count how often we het new IP
  _gotIPStats.setOff();

  _gotIPStats.setOn();

  if (!_netif) { return; }

  if (!_netif->isDefault()) {
    nonDefaultNetworkInterface_gotIP = true;
  }

  for (size_t i = 0; i < NR_ELEMENTS(_dns_cache); ++i) {
    auto tmp = _netif->dnsIP(i);

    _dns_cache[i] = tmp; // Also set the 'empty' ones so we won't set left-over DNS server from when another interface was active.

    if ((tmp != INADDR_NONE) && loglevelActiveFor(LOG_LEVEL_INFO)) {
      addLog(LOG_LEVEL_INFO, strformat(
               F("%s: DNS Cache %d set to %s"),
               esp_netif_get_desc(_netif->netif()),
               i,
               tmp.toString(true).c_str()));
    }
  }

  if (loglevelActiveFor(LOG_LEVEL_INFO)) {
    addLog(LOG_LEVEL_INFO, strformat(
             F("%s: Got IP: %s"),
             esp_netif_get_desc(_netif->netif()),
             _netif->localIP().toString().c_str()
             ));
  }

}

# if FEATURE_USE_IPV6

void NWPluginData_static_runtime::mark_got_IPv6()
{
  _gotIP6Stats.setOn();

  if (!_netif) { return; }

  if (!_netif->isDefault()) {
    nonDefaultNetworkInterface_gotIP = true;
  }

  if (loglevelActiveFor(LOG_LEVEL_INFO)) {
    addLog(LOG_LEVEL_INFO, strformat(
             F("%s: Got IPv6"),
             esp_netif_get_desc(_netif->netif())));
  }

}

# endif // if FEATURE_USE_IPV6

void NWPluginData_static_runtime::mark_lost_IP()
{
  _gotIPStats.setOff();
# if FEATURE_USE_IPV6
  _gotIP6Stats.setOff();
# endif

  if (_netif && loglevelActiveFor(LOG_LEVEL_INFO)) {
    addLog(LOG_LEVEL_INFO, strformat(
             F("%s: Lost IP"),
             esp_netif_get_desc(_netif->netif())));
  }
}

void NWPluginData_static_runtime::mark_connected()
{
  _connectedStats.setOn();

  if (_netif && loglevelActiveFor(LOG_LEVEL_INFO)) {
    addLog(LOG_LEVEL_INFO, strformat(
             F("%s: Connected"),
             esp_netif_get_desc(_netif->netif())));
  }
}

void NWPluginData_static_runtime::mark_disconnected()
{
  _connectedStats.setOff();

  // TODO TD-er: Also clear _gotIPStats and _gotIP6Stats ?

  if (_netif && loglevelActiveFor(LOG_LEVEL_INFO)) {
    addLog(LOG_LEVEL_INFO, strformat(
             F("%s: Disconnected. Connected for: "),
             esp_netif_get_desc(_netif->netif()),
             format_msec_duration_HMS(
               _connectedStats.getLastOnDuration_ms())));
  }
}

} // namespace net
} // namespace ESPEasy

#endif // ifdef ESP32
