#ifndef ESPEASYCORE_ESPEASYWIFI_ABSTRACTED_H
#define ESPEASYCORE_ESPEASYWIFI_ABSTRACTED_H

#include "../../ESPEasy_common.h"


#include "../DataTypes/WiFiConnectionProtocol.h"


#if defined(ESP8266)
#include "../ESPEasyCore/ESPEasyWiFiEvent_ESP8266.h"
# include <ESP8266WiFi.h>
#endif // if defined(ESP8266)
#if defined(ESP32)
#include "../ESPEasyCore/ESPEasyWiFiEvent_ESP32.h"
# include <WiFi.h>
#endif // if defined(ESP32)

namespace ESPEasy_WiFi_abstraction {
bool                   WiFiConnected();

void                   WiFiDisconnect();

bool WifiIsAP(WiFiMode_t wifimode);
bool WifiIsSTA(WiFiMode_t wifimode);


void                   removeWiFiEventHandler();
void                   registerWiFiEventHandler();


WiFiConnectionProtocol getConnectionProtocol();
float                  GetRSSIthreshold(float& maxTXpwr);
#if FEATURE_SET_WIFI_TX_PWR
void                   SetWiFiTXpower(float& dBm);
#endif // if FEATURE_SET_WIFI_TX_PWR

void setConnectionSpeed(bool ForceWiFi_bg_mode);

void setWiFiNoneSleep();
void setWiFiEcoPowerMode();
void setWiFiDefaultPowerMode();


}


#endif // ifndef ESPEASYCORE_ESPEASYWIFI_ABSTRACTED_H
