#include "../Helpers/Networking.h"
#include "../Globals/EventQueue.h"
#include "../Helpers/StringConverter.h"
#include "../Helpers/StringProvider.h"

#include <ArduinoJson.h>

void eventFromResponse(const String& host, const int& httpCode, const String& uri, HTTPClient& http) {
    
  // -------------------------------------------------------------------------------------------Thingspeak
#if FEATURE_THINGSPEAK_EVENT

  // Generate event with the response of a
  // thingspeak request (https://de.mathworks.com/help/thingspeak/readlastfieldentry.html &
  // https://de.mathworks.com/help/thingspeak/readdata.html)
  // e.g. command for a specific field: "sendToHTTP,api.thingspeak.com,80,/channels/1637928/fields/5/last.csv"
  // command for all fields: "sendToHTTP,api.thingspeak.com,80,/channels/1637928/feeds/last.csv"
  // where first eventvalue is the channel number and the second to the nineth event values
  // are the field values
  // Example of the event: "EVENT: ThingspeakReply=1637928,5,24.2,12,900,..."
  //                                                  ^    ^ └------┬------┘
  //                                   channel number ┘    |        └ received values
  //                                                   field number (only available for a "single-value-event")
  // In rules you can grep the reply by "On ThingspeakReply Do ..."
  // -----------------------------------------------------------------------------------------------------------------------------
  // 2024-02-05 - Added the option to get a single value of a field or all values of a channel at a certain time (not only the last entry)
  // Examples:
  // Single channel: "sendtohttp,api.thingspeak.com,80,channels/1637928/fields/1.csv?end=2024-01-01%2023:59:00&results=1"
  // => gets the value of field 1 at (or the last entry before) 23:59:00 of the channel 1637928
  // All channels: "sendtohttp,api.thingspeak.com,80,channels/1637928/feeds.csv?end=2024-01-01%2023:59:00&results=1"
  // => gets the value of each field of the channel 1637928 at (or the last entry before) 23:59:00
  // -----------------------------------------------------------------------------------------------------------------------------

  if ((httpCode == 200) && equals(host, F("api.thingspeak.com")) &&
      (uri.endsWith(F("/last.csv")) || ((uri.indexOf(F("results=1")) >= 0) && (uri.indexOf(F(".csv")) >= 0)))) {
    String result = http.getString();

    result.replace(' ', '_'); // if using a single field with a certain time, the result contains a space and would break the code
    const int posTimestamp = result.lastIndexOf(':');

    if (posTimestamp >= 0) {
      result = parseStringToEndKeepCase(result.substring(posTimestamp), 3);

      if (uri.indexOf(F("fields")) >= 0) {                                        // when there is a single field call add the field number
                                                                                  // before the value
        result = parseStringKeepCase(uri, 4, '/').substring(0, 1) + "," + result; // since the field number is always the fourth part of the
                                                                                  // url and is always a single digit, we can use this to
                                                                                  // extact the fieldnumber
      }
      eventQueue.addMove(strformat(
                           F("ThingspeakReply=%s,%s"),
                           parseStringKeepCase(uri, 2, '/').c_str(),
                           result.c_str()));
    }
  }
#endif // if FEATURE_THINGSPEAK_EVENT

  // ------------------------------------------------------------------------------------------- OpenMeteo
#if FEATURE_OPENMETEO_EVENT

  // Generate an event with the response of an open-meteo request.
  // Example command:
  // sendtohttp,api.open-meteo.com,80,"/v1/forecast?latitude=52.52&longitude=13.41¤t=temperature_2m,weather_code,wind_speed_10m&daily=temperature_2m_max,temperature_2m_min&forecast_days=1"
  // No need for an api key and it is free (daily requests are limited to 10,000 in the free version)
  // Visit the URL (https://open-meteo.com/en/docs) and build your personal URL by selecting the location and values you want to receive.
  // Supported variable kinds are current, hourly, daily!
  // In rules you can grep the reply by the kind of weather variables with "On Openmeteo#<type> Do ..."
  // e.g. "On Openmeteo#current Do ..."
  // Note: hourly and daily results are arrays which can become very long.
  // Best to make seperate calls. Especially for hourly results.

  // define limits
# define WEATHER_KEYS_MAX 10
# define URI_MAX_LENGTH 5000

  if ((httpCode == 200) && equals(host, F("api.open-meteo.com"))) {
    const String str = http.getString();

    if (str.length() > URI_MAX_LENGTH) {
      addLog(LOG_LEVEL_ERROR, strformat(F("Response exceeds %d characters which could cause instabilities or crashes!"), URI_MAX_LENGTH));
    }

    auto processAndQueueParams = [](const String& url, const String& str, const String& eventName) {
                                   // Extract the parameters from the URL
                                   int start = url.indexOf(eventName + '=');

                                   if (start == -1) {
                                     return; // No parameters found for the given eventName
                                   }
                                   start += eventName.length() + 1;
                                   const int end       = url.indexOf('&', start);
                                   const String params = (end == -1) ? url.substring(start) : url.substring(start, end);

                                   if (!params.isEmpty()) {
                                     String keys[WEATHER_KEYS_MAX];
                                     int    keyCount   = 0;
                                     int    startIndex = 0;
                                     int    commaIndex = params.indexOf(',');

                                     // Split and add keys to the array
                                     while (commaIndex != -1) {
                                       if (keyCount >= WEATHER_KEYS_MAX) {
                                         // Stop adding keys if array is full
                                         addLog(LOG_LEVEL_ERROR,
                                                strformat(F(
                                                            "More than %d keys in the URL, this could cause instabilities or crashes! Try to split up the calls.."),
                                                          WEATHER_KEYS_MAX));
                                         break;
                                       }
                                       String key = params.substring(startIndex, commaIndex);
                                       keys[keyCount++] = key;
                                       startIndex       = commaIndex + 1;
                                       commaIndex       = params.indexOf(',', startIndex);
                                     }

                                     // Add the last key
                                     if (keyCount < WEATHER_KEYS_MAX) {
                                       const String lastKey = params.substring(startIndex);
                                       keys[keyCount++] = lastKey;
                                     }

                                     String csv;
                                     const int startStringIndex = str.indexOf(strformat(F("\"%s\":"),
                                                                                        eventName.c_str())) +
                                                                  eventName.length() + 4;

                                     // example( },"current":{"time":... ) we want to start after current":{

                                     const int endStringIndex = str.indexOf('}', startStringIndex);

                                     // ...and want to end before }

                                     for (int i = 0; i < keyCount; i++) // Use keyCount to limit the iteration
                                     {
                                       String key = keys[i];
                                       String value;
                                       int    startIndex = str.indexOf(strformat(F("%s\":"), key.c_str()), startStringIndex);

                                       if (startIndex == -1) {
                                         // Handle case where key is not found
                                         value = F("-256"); // Placeholder value
                                       } else {
                                         int endIndex = 0;

                                         if (!equals(eventName, F("current"))) {
                                           // In daily and hourly, the values are stored in an array
                                           startIndex += key.length() + 3; // Move index past the key
                                           endIndex    = str.indexOf(']', startIndex);
                                         } else {
                                           startIndex += key.length() + 2; // Move index past the key
                                           endIndex    = str.indexOf(',', startIndex);
                                         }

                                         // Find the index of the next comma
                                         if ((endIndex == -1) || (endIndex > str.indexOf("}", startIndex))) {
                                           endIndex = str.indexOf('}', startIndex); // If no comma is found or comma comes after },
                                                                                    // take the rest of the string
                                         }

                                         value = str.substring(startIndex, endIndex);
                                         value.trim(); // Remove any surrounding whitespace
                                       }

                                       if (!csv.isEmpty()) {
                                         csv += ',';
                                       }
                                       csv += value;
                                     }
                                     eventQueue.addMove(strformat(F("OpenMeteo#%s=%s"), eventName.c_str(), csv.c_str()));
                                   }
                                 };

    processAndQueueParams(uri, str, F("current"));
    processAndQueueParams(uri, str, F("hourly"));
    processAndQueueParams(uri, str, F("daily"));
  }

#endif // if FEATURE_OMETEO_EVENT

  // ------------------------------------------------------------------------------------------- Inverter
#if FEATURE_INVERTER_EVENT

  if ((httpCode == 200) && (uri.endsWith(F("GetInverterRealtimeData.cgi?Scope=System"))))
  {
    DynamicJsonDocument *root      = nullptr;
    uint16_t lastJsonMessageLength = 512;

    const String message = http.getString();

    // Cleanup lambda to deallocate resources
    auto cleanupJSON = [&root]() {
                         if (root != nullptr) {
                           root->clear();
                           delete root;
                           root = nullptr;
                         }
                       };

    // Check if root needs resizing or cleanup
    if ((root != nullptr) && (message.length() * 1.5 > lastJsonMessageLength)) {
      cleanupJSON();
    }

    // Resize lastJsonMessageLength if needed
    if (message.length() * 1.5 > lastJsonMessageLength) {
      lastJsonMessageLength = message.length() * 1.5;
    }

    // Allocate memory for root if needed
    if (root == nullptr) {
# ifdef USE_SECOND_HEAP
      HeapSelectIram ephemeral;
# endif                                                                            // ifdef USE_SECOND_HEAP
      root = new (std::nothrow) DynamicJsonDocument(lastJsonMessageLength); // Dynamic allocation
    }

    if (root != nullptr) {
      // Parse the JSON
      DeserializationError error = deserializeJson(*root, message);

      if (!error) {
        int dayEnergy   = (*root)["Body"]["Data"]["DAY_ENERGY"]["Values"]["1"].as<int>();
        int pac         = (*root)["Body"]["Data"]["PAC"]["Values"]["1"].as<int>();
        int totalEnergy = (*root)["Body"]["Data"]["TOTAL_ENERGY"]["Values"]["1"].as<int>();
        int yearEnergy  = (*root)["Body"]["Data"]["YEAR_ENERGY"]["Values"]["1"].as<int>();

        addLog(LOG_LEVEL_INFO, strformat(F("Inverter: Day Energy: %d, PAC: %d, Total Energy: %d, Year Energy: %d"),
                                         dayEnergy,
                                         pac,
                                         totalEnergy,
                                         yearEnergy));
        eventQueue.addMove(strformat(F("InverterReply=%d,%d,%d,%d"), dayEnergy, pac, totalEnergy, yearEnergy));
      } else {
        Serial.println("Failed to parse JSON");
      }

      // Cleanup JSON resources
      cleanupJSON();
    }
  }

#endif // ifdef FEATURE_INVERTER_EVENT
}
