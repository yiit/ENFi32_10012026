#ifdef ESP8266

#include "../ESPEasyCore/ESPEasyNetwork.h"

#include "../ESPEasyCore/ESPEasy_Log.h"
#include "../ESPEasyCore/ESPEasyEth.h"
#include "../../ESPEasy/net/wifi/ESPEasyWifi.h"
#include "../../ESPEasy/net/wifi/ESPEasyWifi_abstracted.h"
#include "../Globals/ESPEasy_time.h"
#include "../../ESPEasy/net/Globals/ESPEasyWiFiEvent.h"
#include "../../ESPEasy/net/Globals/NetworkState.h"
#include "../Globals/Settings.h"

#include "../Helpers/Network.h"
#include "../Helpers/Networking.h"
#include "../Helpers/StringConverter.h"
#include "../Helpers/MDNS_Helper.h"

void setNetworkMedium(NetworkMedium_t new_medium) {
  if (new_medium == NetworkMedium_t::Ethernet) {
    new_medium = NetworkMedium_t::WIFI;
  }

  if (new_medium == active_network_medium) {
    return;
  }

  switch (active_network_medium)
  {
    case NetworkMedium_t::Ethernet:
      break;
    case NetworkMedium_t::WIFI:
      WiFiEventData.timerAPoff.setMillisFromNow(WIFI_AP_OFF_TIMER_DURATION);
      WiFiEventData.timerAPstart.clear();
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
  WiFiConnectRelaxed();
}

bool        NetworkConnected()           { return WiFiConnected(); }

IPAddress   NetworkLocalIP()             { return WiFi.localIP(); }

IPAddress   NetworkSubnetMask()          { return WiFi.subnetMask(); }

IPAddress   NetworkGatewayIP()           { return WiFi.gatewayIP(); }

IPAddress   NetworkDnsIP(uint8_t dns_no) { return WiFi.dnsIP(dns_no); }

MAC_address NetworkMacAddress() {
  MAC_address mac;

  WiFi.macAddress(mac.mac);
  return mac;
}

String NetworkGetHostname() { return String(WiFi.hostname()); }

#if FEATURE_WIFI

MAC_address WifiSoftAPmacAddress() {
  MAC_address mac;

  WiFi.softAPmacAddress(mac.mac);
  return mac;
}

MAC_address WifiSTAmacAddress() {
  MAC_address mac;

  WiFi.macAddress(mac.mac);
  return mac;
}

#endif // if FEATURE_WIFI

#endif // ifdef ESP8266
