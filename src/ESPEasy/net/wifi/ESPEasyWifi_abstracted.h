#ifndef ESPEASYCORE_ESPEASYWIFI_ABSTRACTED_H
#define ESPEASYCORE_ESPEASYWIFI_ABSTRACTED_H

#include "../../../ESPEasy_common.h"

#if FEATURE_WIFI

#include "../wifi/WiFiConnectionProtocol.h"
#include "../wifi/WiFi_STA_connected_state.h"
#include "../wifi/WiFi_State.h"


# if defined(ESP8266)
#  include "../wifi/ESPEasyWiFiEvent_ESP8266.h"
#  include <ESP8266WiFi.h>
# endif // if defined(ESP8266)
# if defined(ESP32)
#  include "../wifi/ESPEasyWiFiEvent_ESP32.h"
#  include <WiFi.h>
#  include <WiFiType.h>
# endif // if defined(ESP32)

namespace ESPEasy {
namespace net {
namespace wifi {


// Call before starting WiFi
bool WiFi_pre_setup();

// Call before setting WiFi into STA mode
bool WiFi_pre_STA_setup();

bool WiFiConnected();

void WiFiDisconnect();

// ********************************************************************************
// Manage Wifi Modes
// ********************************************************************************
bool                       WifiIsAP(WiFiMode_t wifimode);
bool                       WifiIsSTA(WiFiMode_t wifimode);

const __FlashStringHelper* getWifiModeString(WiFiMode_t wifimode);

#if CONFIG_SOC_WIFI_SUPPORT_5G
const __FlashStringHelper* getWifiBandModeString(wifi_band_mode_t wifiBandMode);
#endif

bool                       setSTA(bool enable);
bool                       setAP(bool enable);
bool                       setSTA_AP(bool sta_enable,
                                     bool ap_enable);

bool                       setWifiMode(WiFiMode_t new_mode);


void    WifiScan(bool    async,
                 uint8_t channel = 0);


// ********************************************************************************
// Event handlers
// ********************************************************************************

void                   removeWiFiEventHandler();
void                   registerWiFiEventHandler();


WiFiConnectionProtocol getConnectionProtocol();
float                  GetRSSIthreshold(float& maxTXpwr);

// ********************************************************************************
// Configure WiFi TX power
// ********************************************************************************

# if FEATURE_SET_WIFI_TX_PWR
void SetWiFiTXpower();
void SetWiFiTXpower(float dBm);
void SetWiFiTXpower(float dBm,
                    float rssi);

// Actually set the TX power using platform specific calls.
void doSetWiFiTXpower(float& dBm);
# endif // if FEATURE_SET_WIFI_TX_PWR

int  GetRSSI_quality();

void setConnectionSpeed();
#if CONFIG_SOC_WIFI_SUPPORT_5G
void setConnectionSpeed(bool ForceWiFi_bg_mode, wifi_band_mode_t WiFi_band_mode);
#else
void setConnectionSpeed(bool ForceWiFi_bg_mode);
#endif

void setWiFiNoneSleep();
void setWiFiEcoPowerMode();
void setWiFiDefaultPowerMode();

void setWiFiCountryPolicyManual();

} // namespace wifi
} // namespace net
} // namespace ESPEasy


#endif // if FEATURE_WIFI
#endif // ifndef ESPEASYCORE_ESPEASYWIFI_ABSTRACTED_H
