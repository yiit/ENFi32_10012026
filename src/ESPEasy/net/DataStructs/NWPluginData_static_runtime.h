#pragma once

#include "../DataTypes/NetworkIndex.h"

#include "../../../src/Helpers/LongTermOnOffTimer.h"

#include <IPAddress.h>
#ifdef ESP32
# include <NetworkInterface.h>
#endif

#include <map>

namespace ESPEasy {
namespace net {

struct NWPluginData_static_runtime {
#ifdef ESP32
  NWPluginData_static_runtime(NetworkInterface *netif) : _netif(netif) {}

#else // ifdef ESP32
  NWPluginData_static_runtime(bool isSTA) : _isSTA(isSTA) {}

#endif // ifdef ESP32

  void   clear(networkIndex_t networkIndex = INVALID_NETWORK_INDEX);

  bool   connected() const;

  bool   isDefaultRoute() const;

  bool   hasIP() const;

#if FEATURE_USE_IPV6
  bool   hasIPv6() const;
#endif

  bool   operational() const;

  String statusToString() const;

  // Return true when connected over 1 minute.
  bool   stableConnection() const;


  // =============================================
  // mark_xxx() to act on typical events
  // =============================================
  void mark_start();

  void mark_stop();

  void mark_got_IP();
#if FEATURE_USE_IPV6
  void mark_got_IPv6();
#endif

  void mark_lost_IP();

  void mark_connected();

  void mark_disconnected();


  // =============================================
  // Keep track of connection durations
  // per host/interface
  //
  // When a suggested timeout is not 'acknowledged'
  // a next suggested timeout will be 3x longer.
  // =============================================

  // Return a suggested timeout
  uint32_t getSuggestedTimeout(int      index,
                               uint32_t minimum_timeout) const;

  void     setConnectionDuration(int      index,
                                 uint32_t duration_ms) const;

  // =============================================
  // OnOffTimers for keeping track of:
  // - start/stop of interface
  // - Connected/Disconnected state
  // - GotIP/LostIP
  // - GotIPv6/LostIP
  // =============================================

  LongTermOnOffTimer _startStopStats{};
  LongTermOnOffTimer _connectedStats{};
  LongTermOnOffTimer _gotIPStats{};
#if FEATURE_USE_IPV6
  LongTermOnOffTimer _gotIP6Stats{};
#endif

  // =============================================
  // Cached DNS servers & Route Prio
  // =============================================
#ifdef ESP32
  IPAddress _dns_cache[2]{};
  int       _route_prio = -1;
#endif // ifdef ESP32

private:

#ifdef ESP32
  NetworkInterface *_netif{};
#endif // ifdef ESP32

  networkIndex_t _networkIndex = INVALID_NETWORK_INDEX;
#ifdef ESP8266
  const bool _isSTA;
#endif

  // Store durations it took to connect to some host.
  // This depends on host and interface.
  // Duration is negative when it was suggested but not actually set.
  // Duration is positive when actually being set
  mutable std::map<int, int32_t>_connectDurations;

};


} // namespace net
} // namespace ESPEasy
