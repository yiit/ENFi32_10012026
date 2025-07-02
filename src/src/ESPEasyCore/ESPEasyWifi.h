#ifndef ESPEASY_WIFI_H
#define ESPEASY_WIFI_H

#include "../../ESPEasy_common.h"

#if FEATURE_WIFI

# if defined(ESP8266)
  #  include <ESP8266WiFi.h>
# endif // if defined(ESP8266)
# if defined(ESP32)
  #  include <WiFi.h>
# endif // if defined(ESP32)

# include "../DataTypes/WiFiConnectionProtocol.h"
# include "../DataStructs/WiFi_AP_Candidate.h"

# include "../Helpers/LongTermTimer.h"

# define WIFI_RECONNECT_WAIT                 30000  // in milliSeconds
# define WIFI_AP_OFF_TIMER_DURATION         300000  // in milliSeconds
# if FEATURE_CUSTOM_PROVISIONING
#  define WIFI_CONNECTION_CONSIDERED_STABLE   60000 // in milliSeconds
# else
#  define WIFI_CONNECTION_CONSIDERED_STABLE  60000  // in milliSeconds
# endif // if FEATURE_CUSTOM_PROVISIONING
# define WIFI_ALLOW_AP_AFTERBOOT_PERIOD     5       // in minutes
# define WIFI_SCAN_INTERVAL_AP_USED         125000  // in milliSeconds
# define WIFI_SCAN_INTERVAL_MINIMAL          60000  // in milliSeconds


# ifdef ESPEASY_WIFI_CLEANUP_WORK_IN_PROGRESS


# endif // ESPEASY_WIFI_CLEANUP_WORK_IN_PROGRESS

bool WiFiConnected();
void WiFiConnectRelaxed();
void AttemptWiFiConnect();
bool prepareWiFi();
bool checkAndResetWiFi();
void resetWiFi();
void initWiFi();
void loopWiFi();

# if FEATURE_SET_WIFI_TX_PWR
void  SetWiFiTXpower();
void  SetWiFiTXpower(float dBm); // 0-20.5
void  SetWiFiTXpower(float dBm,
                     float rssi);
# endif // if FEATURE_SET_WIFI_TX_PWR
float GetRSSIthreshold(float& maxTXpwr);

// Return some quality based on RSSI.
// <-97 => 0 , >-50 => 10
// -97 ... -50 => 1 ... 9
int                    GetRSSI_quality();
WiFiConnectionProtocol getConnectionProtocol();
# ifdef ESP32

// TSF time is 64-bit timer in usec, sent by the AP along with other packets.
// On tested access points, this seems to be the uptime in usec.
// Could be used among nodes connected to the same AP to increase time sync accuracy.
int64_t WiFi_get_TSF_time();
# endif // ifdef ESP32
void    WifiDisconnect();
bool    WiFiScanAllowed();
void    WiFiScan_log_to_serial();

void    setAPinternal(bool enable); // FIXME TD-er: Move to ESPEasyWifi_abstracted...

bool    WiFiUseStaticIP();
bool    wifiAPmodeActivelyUsed();
void    setupStaticIPconfig();
String  formatScanResult(int           i,
                         const String& separator);
String  formatScanResult(int           i,
                         const String& separator,
                         int32_t     & rssi);

void logConnectionStatus();

#endif // if FEATURE_WIFI

#endif // ESPEASY_WIFI_H
