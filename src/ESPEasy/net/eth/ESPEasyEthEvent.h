#pragma once

#include "../../../ESPEasy_common.h"

#ifdef ESP32
# if FEATURE_ETHERNET

namespace ESPEasy {
namespace net {
namespace eth {

// ********************************************************************************
// Functions called on Eth events.
// Make sure not to call anything in these functions that result in delay() or yield()
// ********************************************************************************
#  include <WiFi.h>
#  include <WiFiType.h>
void EthEvent(WiFiEvent_t          event,
              arduino_event_info_t info);

} // namespace eth
} // namespace net
} // namespace ESPEasy

# endif // if FEATURE_ETHERNET
#endif // ifdef ESP32
