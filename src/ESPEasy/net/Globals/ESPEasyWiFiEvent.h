#pragma once

#include "../../../ESPEasy_common.h"

#if FEATURE_WIFI

# include "../DataStructs/WiFiEventData.h"


# include <IPAddress.h>
# include <stdint.h>


# ifdef ESP32
#  include <esp_event.h>
#  include <WiFiGeneric.h>
#  include <WiFiType.h>

# endif // ifdef ESP32

# ifdef ESP8266
#  include <ESP8266WiFiGeneric.h>
#  include <ESP8266WiFiType.h>
class IPAddress;

extern WiFiEventHandler stationConnectedHandler;
extern WiFiEventHandler stationDisconnectedHandler;
extern WiFiEventHandler stationGotIpHandler;
extern WiFiEventHandler stationModeDHCPTimeoutHandler;
extern WiFiEventHandler stationModeAuthModeChangeHandler;
extern WiFiEventHandler APModeStationConnectedHandler;
extern WiFiEventHandler APModeStationDisconnectedHandler;
# endif // ifdef ESP8266


extern WiFiEventData_t WiFiEventData;


#endif // if FEATURE_WIFI
