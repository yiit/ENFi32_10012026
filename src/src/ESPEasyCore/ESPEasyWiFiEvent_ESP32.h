#ifndef ESPEASY_WIFI_EVENT_ESP32_H
#define ESPEASY_WIFI_EVENT_ESP32_H

#ifdef ESP32

#include "../../ESPEasy_common.h"

#include <IPAddress.h>

// ********************************************************************************

// Work-around for setting _useStaticIP
// See reported issue: https://github.com/esp8266/Arduino/issues/4114
// ********************************************************************************
#include <IPAddress.h>
#include <WiFiSTA.h>
#include <WiFi.h>
class WiFi_Access_Static_IP : public WiFiSTAClass {
public:

  void set_use_static_ip(bool enabled);
};

void setUseStaticIP(bool enabled);


// ********************************************************************************
// Functions called on events.
// Make sure not to call anything in these functions that result in delay() or yield()
// ********************************************************************************
 #if ESP_IDF_VERSION_MAJOR > 3
  #include <WiFiType.h>
  void WiFiEvent(WiFiEvent_t event, arduino_event_info_t info);
 #else
  void WiFiEvent(system_event_id_t event, system_event_info_t info);
 #endif

#endif

#endif