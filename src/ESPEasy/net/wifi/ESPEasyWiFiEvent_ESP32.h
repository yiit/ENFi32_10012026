#pragma once

# include "../../../ESPEasy_common.h"

#if FEATURE_WIFI
#ifdef ESP32

# include <IPAddress.h>

// ********************************************************************************

// Work-around for setting _useStaticIP
// See reported issue: https://github.com/esp8266/Arduino/issues/4114
// ********************************************************************************
# include <IPAddress.h>
# include <WiFiSTA.h>
# include <WiFi.h>
# include <WiFiType.h>

namespace ESPEasy {
namespace net {
namespace wifi {

class WiFi_Access_Static_IP : public WiFiSTAClass
{
public:

  void set_use_static_ip(bool enabled);
};

void setUseStaticIP(bool enabled);


// ********************************************************************************
// Functions called on events.
// Make sure not to call anything in these functions that result in delay() or yield()
// ********************************************************************************
void WiFiEvent(WiFiEvent_t          event,
               arduino_event_info_t info);

}
}
}

#endif // ifdef ESP32
#endif
