#include "../net/ESPEasyNetwork.h"

#include "../../ESPEasy/net/wifi/ESPEasyWifi.h"


#include "../../ESPEasy/net/Globals/NetworkState.h"
#include "../../src/Globals/Settings.h"

#include "../../src/Globals/ESPEasy_time.h"

#include "../../src/Helpers/StringConverter.h"
#include "../../src/Helpers/MDNS_Helper.h"

namespace ESPEasy {
namespace net {

// ********************************************************************************
// Determine Wifi AP name to set. (also used for mDNS)
// ********************************************************************************
String NetworkGetHostNameFromSettings(bool force_add_unitnr)
{
  if (force_add_unitnr) { return Settings.getHostname(true); }
  return Settings.getHostname();
}

String NetworkCreateRFCCompliantHostname(bool force_add_unitnr) {
  String hostname(NetworkGetHostNameFromSettings(force_add_unitnr));

  // Create hostname with - instead of spaces

  // See RFC952.
  // Allowed chars:
  // * letters (a-z, A-Z)
  // * numerals (0-9)
  // * Hyphen (-)
  replaceUnicodeByChar(hostname, '-');

  for (size_t i = 0; i < hostname.length(); ++i) {
    const char c = hostname[i];

    if (!isAlphaNumeric(c)) {
      hostname[i] = '-';
    }
  }

  // May not start or end with a hyphen
  const String dash('-');

  while (hostname.startsWith(dash)) {
    hostname = hostname.substring(1);
  }

  while (hostname.endsWith(dash)) {
    hostname = hostname.substring(0, hostname.length() - 1);
  }

  // May not contain only numerals
  bool onlyNumerals = true;

  for (size_t i = 0; onlyNumerals && i < hostname.length(); ++i) {
    const char c = hostname[i];

    if (!isdigit(c)) {
      onlyNumerals = false;
    }
  }

  if (onlyNumerals) {
    hostname = concat(F("ESPEasy-"), hostname);
  }

  if (hostname.length() > 24) {
    hostname = hostname.substring(0, 24);
  }

  return hostname;
}

void CheckRunningServices() {
  // First try to get the time, since that may be used in logs
  if (Settings.UseNTP() && (node_time.getTimeSource() > timeSource_t::NTP_time_source)) {
    node_time.lastNTPSyncTime_ms = 0;
    node_time.initTime();
  }
#if FEATURE_WIFI
# if FEATURE_SET_WIFI_TX_PWR

  if (active_network_medium == NetworkMedium_t::WIFI)
  {
    ESPEasy::net::wifi::SetWiFiTXpower();
  }
# endif // if FEATURE_SET_WIFI_TX_PWR
#endif // if FEATURE_WIFI

#if FEATURE_MDNS
  set_mDNS();
  #endif
}

}
}
