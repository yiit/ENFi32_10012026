#include "../eth/ESPEasyEthEvent.h"

#ifdef ESP32
# if FEATURE_ETHERNET
#  include <ETH.h>

#  include "../eth/ESPEasyEth.h"
#  include "../Globals/ESPEasyEthEvent.h"


// ********************************************************************************
// Functions called on events.  ** WARNING **
// Make sure not to call anything in these functions that result in delay() or yield()
// ********************************************************************************

#  include <WiFi.h>

namespace ESPEasy {
namespace net {
namespace eth {

void EthEvent(WiFiEvent_t event, arduino_event_info_t info) {
  switch (event)
  {
    case ARDUINO_EVENT_ETH_START:

      if (ethPrepare()) {
        //      addLog(LOG_LEVEL_INFO, F("ETH event: Started"));
        //    } else {
        //      addLog(LOG_LEVEL_ERROR, F("ETH event: Could not prepare ETH!"));
      }
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      //      addLog(LOG_LEVEL_INFO, F("ETH event: Connected"));
      EthEventData.markConnected();
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      EthEventData.markGotIP();

      //      addLog(LOG_LEVEL_INFO,  F("ETH event: Got IP"));
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      //      addLog(LOG_LEVEL_ERROR, F("ETH event: Disconnected"));
      EthEventData.markDisconnect();
      break;
    case ARDUINO_EVENT_ETH_STOP:
      //      addLog(LOG_LEVEL_INFO, F("ETH event: Stopped"));
      break;
    case ARDUINO_EVENT_ETH_GOT_IP6:
    #  if FEATURE_USE_IPV6
    {
      ip_event_got_ip6_t *event = static_cast<ip_event_got_ip6_t *>(&info.got_ip6);
      IPAddress ip(IPv6, (const uint8_t *)event->ip6_info.ip.addr, event->ip6_info.ip.zone);
      EthEventData.markGotIPv6(ip);

      //      addLog(LOG_LEVEL_INFO, String(F("ETH event: Got IP6 ")) + ip.toString(true));
    }
    #  else // if FEATURE_USE_IPV6

      //    addLog(LOG_LEVEL_INFO, F("ETH event: Got IP6"));
    #  endif // if FEATURE_USE_IPV6
      break;
    default:
    {
      break;
    }
  }
}

} // namespace eth
} // namespace net
} // namespace ESPEasy

# endif // if FEATURE_ETHERNET

#endif // ifdef ESP32
