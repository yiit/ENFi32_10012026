#pragma once

#include "../../../ESPEasy_common.h"

#if FEATURE_WIFI
# ifdef ESP32

#  include "../../../src/Helpers/LongTermOnOffTimer.h"

#  include <IPAddress.h>

// ********************************************************************************

// Work-around for setting _useStaticIP
// See reported issue: https://github.com/esp8266/Arduino/issues/4114
// ********************************************************************************
#  include <IPAddress.h>
#  include <WiFiSTA.h>
#  include <WiFi.h>
#  include <WiFiType.h>

# include "../wifi/WiFiDisconnectReason.h"


namespace ESPEasy {
namespace net {
namespace wifi {


class ESPEasyWiFi_STA_EventHandler
{
public:

  ESPEasyWiFi_STA_EventHandler();
  ~ESPEasyWiFi_STA_EventHandler();

  static bool initialized();

  LongTermOnOffTimer   getEnabled_OnOffTimer() const;
  LongTermOnOffTimer   getConnected_OnOffTimer() const;
  LongTermOnOffTimer   getGotIp_OnOffTimer() const;
#  if FEATURE_USE_IPV6
  LongTermOnOffTimer   getGotIp6_OnOffTimer() const;
#  endif

  WiFiDisconnectReason getLastDisconnectReason() const;

  uint8_t getAuthMode() const;

  bool                 restore_dns_from_cache() const;

private:

  // ********************************************************************************
  // Functions called on events.
  // Make sure not to call anything in these functions that result in delay() or yield()
  // ********************************************************************************
  static void WiFiEvent(WiFiEvent_t          event,
                        arduino_event_info_t info);

  network_event_handle_t nw_event_id = 0;


}; // class ESPEasyWiFi_STA_EventHandler

} // namespace wifi
} // namespace net
} // namespace ESPEasy

# endif // ifdef ESP32
#endif // if FEATURE_WIFI
