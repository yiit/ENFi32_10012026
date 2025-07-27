#include "../wifi/ESPEasyWiFiEvent_ESP32.h"

#if FEATURE_WIFI

# ifdef ESP32
#  if FEATURE_ETHERNET
#   include <ETH.h>
#  endif // if FEATURE_ETHERNET
#  if FEATURE_PPP_MODEM
#   include <PPP.h>
#  endif

#  include "../../../src/DataStructs/RTCStruct.h"

#  include "../../../src/DataTypes/ESPEasyTimeSource.h"

#  include "../../../src/ESPEasyCore/ESPEasyEth.h"
#  include "../../../src/ESPEasyCore/ESPEasy_Log.h"
#  include "../../../src/ESPEasyCore/ESPEasyNetwork.h"

#  include "../wifi/ESPEasyWifi.h"
#  include "../wifi/ESPEasyWifi_ProcessEvent.h"

#  include "../Globals/ESPEasyWiFiEvent.h"
#  include "../Globals/NetworkState.h"
#  include "../../../src/Globals/RTC.h"
#  include "../../../src/Globals/WiFi_AP_Candidates.h"

#  include "../../../src/Helpers/ESPEasy_time_calc.h"
#  include "../../../src/Helpers/StringConverter.h"


#  if FEATURE_ETHERNET
#   include "../Globals/ESPEasyEthEvent.h"
#  endif // if FEATURE_ETHERNET

namespace ESPEasy {
namespace net {
namespace wifi {

void setUseStaticIP(bool enabled) {}

// ********************************************************************************
// Functions called on events.
// Make sure not to call anything in these functions that result in delay() or yield()
// ********************************************************************************
#  include <WiFi.h>

static bool ignoreDisconnectEvent = false;

void WiFiEvent(WiFiEvent_t event_id, arduino_event_info_t info) {
  switch (event_id)
  {
    case ARDUINO_EVENT_WIFI_READY:
      // ESP32 WiFi ready
      break;
    case ARDUINO_EVENT_WIFI_STA_START:
    #  ifndef BUILD_NO_DEBUG

      // addLog(LOG_LEVEL_INFO, F("WiFi : Event STA Started"));
    #  endif // ifndef BUILD_NO_DEBUG
      break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
    #  ifndef BUILD_NO_DEBUG

      // addLog(LOG_LEVEL_INFO, F("WiFi : Event STA Stopped"));
    #  endif // ifndef BUILD_NO_DEBUG
      break;
    case ARDUINO_EVENT_WIFI_OFF:
    #  ifndef BUILD_NO_DEBUG

      // addLog(LOG_LEVEL_INFO, F("WiFi : Event WIFI off"));
    #  endif // ifndef BUILD_NO_DEBUG
      break;

    case ARDUINO_EVENT_WIFI_AP_START:
    #  ifndef BUILD_NO_DEBUG

      // addLog(LOG_LEVEL_INFO, F("WiFi : Event AP Started"));
    #  endif // ifndef BUILD_NO_DEBUG
      break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
    #  ifndef BUILD_NO_DEBUG

      // addLog(LOG_LEVEL_INFO, F("WiFi : Event AP Stopped"));
    #  endif // ifndef BUILD_NO_DEBUG
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      // ESP32 station lost IP and the IP is reset to 0
      #  if FEATURE_ETHERNET

      if (active_network_medium == NetworkMedium_t::Ethernet) {
        // DNS records are shared among WiFi and Ethernet (very bad design!)
        // So we must restore the DNS records for Ethernet in case we started with WiFi and then plugged in Ethernet.
        // As soon as WiFi is turned off, the DNS entry for Ethernet is cleared.
        EthEventData.markLostIP();
      }
      #  endif // if FEATURE_ETHERNET
      WiFiEventData.markLostIP();
      #  ifndef BUILD_NO_DEBUG

      // addLog(LOG_LEVEL_INFO,

      /*
         active_network_medium == NetworkMedium_t::Ethernet ?
         F("ETH : Event Lost IP") :
       */

      //         F("WiFi : Event Lost IP"));
      #  endif // ifndef BUILD_NO_DEBUG
      break;

    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
      // Receive probe request packet in soft-AP interface
      // TODO TD-er: Must implement like onProbeRequestAPmode for ESP8266
      #  ifndef BUILD_NO_DEBUG

      // addLog(LOG_LEVEL_INFO, F("WiFi : Event AP got probed"));
      #  endif // ifndef BUILD_NO_DEBUG
      break;

    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
      WiFiEventData.setAuthMode(info.wifi_sta_authmode_change.new_mode);
      break;

    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
    {
      char ssid_copy[33]; // Ensure space for maximum len SSID (32) plus trailing 0
      memcpy(ssid_copy, info.wifi_sta_connected.ssid, info.wifi_sta_connected.ssid_len);
      ssid_copy[32] = 0;  // Potentially add 0-termination if none present earlier
      WiFiEventData.markConnected((const char *)ssid_copy, info.wifi_sta_connected.bssid, info.wifi_sta_connected.channel);
      WiFiEventData.setAuthMode(info.wifi_sta_connected.authmode);

      // addLog(LOG_LEVEL_INFO, F("WiFi : Event WIFI_STA_CONNECTED"));
      break;
    }
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      WiFiEventData.markDisconnect(static_cast<WiFiDisconnectReason>(info.wifi_sta_disconnected.reason));
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    {
      // FIXME TD-er: Must also check event->esp_netif to see which interface got this event.
      ignoreDisconnectEvent = false;
      ip_event_got_ip_t *event = static_cast<ip_event_got_ip_t *>(&info.got_ip);
      const IPAddress    ip(event->ip_info.ip.addr);
      const IPAddress    netmask(event->ip_info.netmask.addr);
      const IPAddress    gw(event->ip_info.gw.addr);
      WiFiEventData.markGotIP(ip, netmask, gw);
      break;
    }
    #  if FEATURE_USE_IPV6
    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
    {
      ip_event_got_ip6_t *event = static_cast<ip_event_got_ip6_t *>(&info.got_ip6);
      const IPAddress     ip(IPv6, (const uint8_t *)event->ip6_info.ip.addr, event->ip6_info.ip.zone);
      WiFiEventData.markGotIPv6(ip);
      break;
    }
    case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
      addLog(LOG_LEVEL_INFO, F("WIFI : AP got IP6"));
      break;
    #  endif // if FEATURE_USE_IPV6
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
      addLog(LOG_LEVEL_INFO, F("WIFI : AP assigned IP to STA"));
      break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      WiFiEventData.markConnectedAPmode(info.wifi_ap_staconnected.mac);
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      WiFiEventData.markDisconnectedAPmode(info.wifi_ap_stadisconnected.mac);
      break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
      WiFiEventData.processedScanDone = false;
      break;
#  if FEATURE_ETHERNET
    case ARDUINO_EVENT_ETH_START:
    case ARDUINO_EVENT_ETH_CONNECTED:
    case ARDUINO_EVENT_ETH_GOT_IP:
    case ARDUINO_EVENT_ETH_DISCONNECTED:
    case ARDUINO_EVENT_ETH_STOP:
    case ARDUINO_EVENT_ETH_GOT_IP6:

      // Handled in EthEvent
      break;
#  endif // FEATURE_ETHERNET

#  if FEATURE_PPP_MODEM
    case ARDUINO_EVENT_PPP_START:     addLog(LOG_LEVEL_INFO, F("PPP Started"));
      break;
    case ARDUINO_EVENT_PPP_CONNECTED: addLog(LOG_LEVEL_INFO, F("PPP Connected"));
      break;
    case ARDUINO_EVENT_PPP_GOT_IP:
      addLog(LOG_LEVEL_INFO, F("PPP Got IP"));
      PPP.setDefault();

      if (WiFi.AP.enableNAPT(true)) {
        addLog(LOG_LEVEL_INFO, F("WiFi.AP.enableNAPT"));
      }
      break;
    case ARDUINO_EVENT_PPP_LOST_IP:
      addLog(LOG_LEVEL_INFO, F("PPP Lost IP"));
      WiFi.AP.enableNAPT(false);
      break;
    case ARDUINO_EVENT_PPP_DISCONNECTED:
      addLog(LOG_LEVEL_INFO, F("PPP Disconnected"));
      WiFi.AP.enableNAPT(false);
      break;
    case ARDUINO_EVENT_PPP_STOP: addLog(LOG_LEVEL_INFO, F("PPP Stopped"));
      break;

#  endif // if FEATURE_PPP_MODEM

    default:
    {
      addLogMove(LOG_LEVEL_ERROR, concat(F("UNKNOWN WIFI/ETH EVENT: "),  event_id));
      break;
    }
  }
}

}
}
}
# endif // ifdef ESP32
#endif // if FEATURE_WIFI
