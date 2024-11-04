#ifdef ESP32

#include "../ESPEasyCore/ESPEasyNetwork.h"

#include "../ESPEasyCore/ESPEasy_Log.h"
#include "../ESPEasyCore/ESPEasyEth.h"
#include "../ESPEasyCore/ESPEasyWifi.h"
#include "../ESPEasyCore/ESPEasyWifi_abstracted.h"
#include "../Globals/ESPEasy_time.h"
#include "../Globals/ESPEasyWiFiEvent.h"
#include "../Globals/NetworkState.h"
#include "../Globals/Settings.h"

#include "../Globals/ESPEasyEthEvent.h"

#include "../Helpers/Network.h"
#include "../Helpers/Networking.h"
#include "../Helpers/StringConverter.h"
#include "../Helpers/MDNS_Helper.h"

#include <NetworkManager.h>

#if FEATURE_ETHERNET
# include <ETH.h>
#endif

void setNetworkMedium(NetworkMedium_t new_medium) {
#if !(FEATURE_ETHERNET)

  if (new_medium == NetworkMedium_t::Ethernet) {
    new_medium = NetworkMedium_t::WIFI;
  }
#endif // if !(FEATURE_ETHERNET)

  if (new_medium == active_network_medium) {
    return;
  }

  switch (active_network_medium)
  {
    case NetworkMedium_t::WIFI:
      WiFi.STA.end();
      break;
    case NetworkMedium_t::Ethernet:
#if FEATURE_ETHERNET
      ETH.end();
#endif // if FEATURE_ETHERNET
      break;
    case NetworkMedium_t::NotSet:
      break;
  }

  switch (new_medium)
  {
    case NetworkMedium_t::WIFI:
      WiFi.STA.setDefault();
      break;
    case NetworkMedium_t::Ethernet:
#if FEATURE_ETHERNET
      ETH.setDefault();
#endif // if FEATURE_ETHERNET
      break;
    case NetworkMedium_t::NotSet:
      break;
  }
  statusLED(true);
  active_network_medium = new_medium;
  addLog(LOG_LEVEL_INFO, concat(F("Set Network mode: "), toString(active_network_medium)));
}

/*********************************************************************************************\
   Ethernet or Wifi Support for ESP32 Build flag FEATURE_ETHERNET
\*********************************************************************************************/
void NetworkConnectRelaxed() {
  if (NetworkConnected()) { return; }
#if FEATURE_ETHERNET

  if (active_network_medium == NetworkMedium_t::Ethernet) {
    if (ETHConnectRelaxed()) {
      return;
    }

    // Failed to start the Ethernet network, probably not present of wrong parameters.
    // So set the runtime active medium to WiFi to try connecting to WiFi or at least start the AP.
    setNetworkMedium(NetworkMedium_t::WIFI);
  }
#endif // if FEATURE_ETHERNET

  // Failed to start the Ethernet network, probably not present of wrong parameters.
  // So set the runtime active medium to WiFi to try connecting to WiFi or at least start the AP.
  WiFiConnectRelaxed();
}

bool NetworkConnected() {
  auto network_if = Network.getDefaultInterface();

  if (network_if == nullptr) {
    return false;
  }
  return network_if->connected();
}

IPAddress NetworkLocalIP() {
  auto network_if = Network.getDefaultInterface();

  if (network_if == nullptr) {
    return IPAddress();
  }
  return network_if->localIP();
}

IPAddress NetworkSubnetMask() {
  auto network_if = Network.getDefaultInterface();

  if (network_if == nullptr) {
    return IPAddress();
  }
  return network_if->subnetMask();
}

IPAddress NetworkGatewayIP() {
  auto network_if = Network.getDefaultInterface();

  if (network_if == nullptr) {
    return IPAddress();
  }
  return network_if->gatewayIP();
}

IPAddress NetworkDnsIP(uint8_t dns_no) {
  auto network_if = Network.getDefaultInterface();

  if (network_if == nullptr) {
    return IPAddress();
  }
  return network_if->dnsIP(dns_no);
}

#if FEATURE_USE_IPV6

IPAddress NetworkLocalIP6() {
  auto network_if = Network.getDefaultInterface();

  if (network_if == nullptr) {
    return IN6ADDR_ANY;
  }
  return network_if->linkLocalIPv6();
}

IPAddress NetworkGlobalIP6() {
  auto network_if = Network.getDefaultInterface();

  if (network_if == nullptr) {
    return IN6ADDR_ANY;
  }
  return network_if->globalIPv6();
}

IP6Addresses_t NetworkAllIPv6() {
  IP6Addresses_t addresses;
  auto network_if = Network.getDefaultInterface();

  if (network_if != nullptr) {
    esp_ip6_addr_t esp_ip6_addr[LWIP_IPV6_NUM_ADDRESSES]{};

    int count = esp_netif_get_all_ip6(network_if->netif(), esp_ip6_addr);

    for (int i = 0; i < count; ++i) {
      addresses.emplace_back(IPv6, (const uint8_t *)esp_ip6_addr[i].addr, esp_ip6_addr[i].zone);
    }
  }

  return addresses;
}

bool IPv6_from_MAC(const MAC_address& mac, IPAddress& ipv6)
{
  if (ipv6 == IN6ADDR_ANY) { return false; }
  int index_offset = 8;

  for (int i = 0; i < 6; ++i, ++index_offset) {
    ipv6[index_offset] = mac.mac[i];

    if (i == 0) {
      // invert bit 2
      bitToggle(ipv6[index_offset], 1);
    }

    if (i == 2) {
      ipv6[++index_offset] = 0xFF;
      ipv6[++index_offset] = 0xFE;
    }
  }

  /*
     addLog(LOG_LEVEL_INFO, strformat(
       F("IPv6_from_MAC: Mac %s IP %s"),
       mac.toString().c_str(),
       ipv6.toString(true).c_str()
       ));
   */
  return true;
}

bool is_IPv6_based_on_MAC(const MAC_address& mac, const IPAddress& ipv6)
{
  IPAddress tmp = ipv6;

  if (IPv6_from_MAC(mac, tmp)) {
    return ipv6 == tmp;
  }
  return false;
}

bool IPv6_link_local_from_MAC(const MAC_address& mac, IPAddress& ipv6)
{
  ipv6 = NetworkLocalIP6();
  return IPv6_from_MAC(mac, ipv6);
}

bool is_IPv6_link_local_from_MAC(const MAC_address& mac) { return is_IPv6_based_on_MAC(mac, NetworkLocalIP6()); }

// Assume we're in the same subnet, thus use our own IPv6 global address
bool IPv6_global_from_MAC(const MAC_address& mac, IPAddress& ipv6)
{
  ipv6 = NetworkGlobalIP6();
  return IPv6_from_MAC(mac, ipv6);
}

bool is_IPv6_global_from_MAC(const MAC_address& mac) { return is_IPv6_based_on_MAC(mac, NetworkGlobalIP6()); }

#endif // if FEATURE_USE_IPV6

MAC_address NetworkMacAddress() {
  MAC_address res;
  auto network_if = Network.getDefaultInterface();

  if (network_if != nullptr) {
    network_if->macAddress(res.mac);
  }
  return res;
}

String NetworkGetHostname() {
  auto network_if = Network.getDefaultInterface();

  if (network_if != nullptr) {
    return network_if->getHostname();
  }
  return EMPTY_STRING;
}

#if FEATURE_WIFI
MAC_address WifiSoftAPmacAddress() {
  MAC_address mac;

  WiFi.AP.macAddress(mac.mac);
  return mac;
}

MAC_address WifiSTAmacAddress() {
  MAC_address mac;

  WiFi.macAddress(mac.mac);
  return mac;
}
#endif

#if FEATURE_ETHERNET

bool EthFullDuplex()
{
  if (EthEventData.ethInitSuccess) {
    return ETH.fullDuplex();
  }
  return false;
}

bool EthLinkUp()
{
  if (EthEventData.ethInitSuccess) {
    return ETH.linkUp();
  }
  return false;
}

uint8_t EthLinkSpeed()
{
  if (EthEventData.ethInitSuccess) {
    return ETH.linkSpeed();
  }
  return 0;
}

#endif // if FEATURE_ETHERNET

#endif // ifdef ESP32
