#pragma once

#include "../../../ESPEasy_common.h"

#if FEATURE_WIFI


# ifdef ESP8266


#  include <IPAddress.h>

// ********************************************************************************

// Work-around for setting _useStaticIP
// See reported issue: https://github.com/esp8266/Arduino/issues/4114
// ********************************************************************************

#  include <ESP8266WiFi.h>
#  include <ESP8266WiFiSTA.h>

namespace ESPEasy {
namespace net {
namespace wifi {

class WiFi_Access_Static_IP : public ESP8266WiFiSTAClass
{
public:

  void set_use_static_ip(bool enabled);
};

void setUseStaticIP(bool enabled);


// ********************************************************************************
// Functions called on events.
// Make sure not to call anything in these functions that result in delay() or yield()
// ********************************************************************************
void onConnected(const WiFiEventStationModeConnected& event);

void onDisconnect(const WiFiEventStationModeDisconnected& event);

void onGotIP(const WiFiEventStationModeGotIP& event);

void onDHCPTimeout();

void onConnectedAPmode(const WiFiEventSoftAPModeStationConnected& event);

void onDisconnectedAPmode(const WiFiEventSoftAPModeStationDisconnected& event);

void onStationModeAuthModeChanged(const WiFiEventStationModeAuthModeChanged& event);

#  if FEATURE_ESP8266_DIRECT_WIFI_SCAN
void onWiFiScanDone(void  *arg,
                    STATUS status);
#  endif // if FEATURE_ESP8266_DIRECT_WIFI_SCAN

}
}
}

# endif // ifdef ESP8266

#endif // if FEATURE_WIFI
