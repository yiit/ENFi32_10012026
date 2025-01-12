
#include "../Helpers/_CPlugin_Helper_mqtt.h"

#if FEATURE_MQTT
# include "../Commands/ExecuteCommand.h"

# if FEATURE_MQTT_DISCOVER
#  include "../Globals/MQTT.h"
#  include "../Helpers/StringGenerator_System.h"
# endif // if FEATURE_MQTT_DISCOVER

/***************************************************************************************
 * Parse MQTT topic for /cmd and /set ending to handle commands or TaskValueSet
 * Special C014 case: handleCmd = false and handleSet is true, so *only* pluginID 33 & 86 are accepted
 **************************************************************************************/
bool MQTT_handle_topic_commands(struct EventStruct *event,
                                bool                handleCmd,
                                bool                handleSet,
                                bool                tryRemoteConfig) {
  bool handled = false;

  // Topic  : event->String1
  // Message: event->String2
  String cmd;
  int    lastindex           = event->String1.lastIndexOf('/');
  const String lastPartTopic = event->String1.substring(lastindex + 1);

  if (!handled && handleCmd && equals(lastPartTopic, F("cmd"))) {
    // Example:
    // Topic: ESP_Easy/Bathroom_pir_env/cmd
    // Message: gpio,14,0
    // Full command:  gpio,14,0

    cmd = event->String2;

    // SP_C005a: string= ;cmd=gpio,12,0 ;taskIndex=12 ;string1=ESPT12/cmd ;string2=gpio,12,0
    handled = true;
  }

  if (!handled && handleSet && equals(lastPartTopic, F("set"))) {
    // Example:
    // Topic: ESP_Easy/DummyTask/DummyVar/set
    // Message: 14
    // Full command: TaskValueSet,DummyTask,DummyVar,14
    const String topic = event->String1.substring(0, lastindex);
    lastindex = topic.lastIndexOf('/');

    if (lastindex > -1) {
      String taskName        = topic.substring(0, lastindex);
      const String valueName = topic.substring(lastindex + 1);
      lastindex = taskName.lastIndexOf('/');

      if (lastindex > -1) {
        taskName = taskName.substring(lastindex + 1);

        const taskIndex_t    taskIndex    = findTaskIndexByName(taskName);
        const deviceIndex_t  deviceIndex  = getDeviceIndex_from_TaskIndex(taskIndex);
        const taskVarIndex_t taskVarIndex = event->Par2 - 1;
        uint8_t valueNr;

        if (validDeviceIndex(deviceIndex) && validTaskVarIndex(taskVarIndex)) {
          const int pluginID = Device[deviceIndex].Number;

          # ifdef USES_P033

          if ((pluginID == 33) || // Plugin 33 Dummy Device,
              // backward compatible behavior: if handleCmd = true then execute TaskValueSet regardless of AllowTaskValueSetAllPlugins
              ((handleCmd || Settings.AllowTaskValueSetAllPlugins()) && (pluginID != 86))) {
            // TaskValueSet,<task/device nr>,<value nr>,<value/formula (!ToDo) >, works only with new version of P033!
            valueNr = findDeviceValueIndexByName(valueName, taskIndex);

            if (validTaskVarIndex(valueNr)) { // value Name identified
              // Set a Dummy Device Value, device Number, var number and argument
              cmd     = strformat(F("TaskValueSet,%d,%d,%s"), taskIndex + 1, valueNr + 1, event->String2.c_str());
              handled = true;
            }
          }
          # endif // ifdef USES_P033
          # if defined(USES_P033) && defined(USES_P086)
          else
          # endif // if defined(USES_P033) && defined(USES_P086)
          # ifdef USES_P086

          if (pluginID == 86) { // Plugin 86 Homie receiver. Schedules the event defined in the plugin.
            // Does NOT store the value.
            // Use HomieValueSet to save the value. This will acknowledge back to the controller too.
            valueNr = findDeviceValueIndexByName(valueName, taskIndex);

            if (validTaskVarIndex(valueNr)) {
              cmd = strformat(F("event,%s="), valueName.c_str());

              if (Settings.TaskDevicePluginConfig[taskIndex][valueNr] == 3) {   // Quote String parameters. PLUGIN_086_VALUE_STRING
                cmd += wrapWithQuotes(event->String2);
              } else {
                if (Settings.TaskDevicePluginConfig[taskIndex][valueNr] == 4) { // Enumeration parameter, find Number of item.
                                                                                // PLUGIN_086_VALUE_ENUM
                  const String enumList = ExtraTaskSettings.TaskDeviceFormula[taskVarIndex];
                  int i                 = 1;
                  String part           = parseStringKeepCase(enumList, i);

                  while (!part.isEmpty()) {                      // lookup result in enum List, keep it backward compatible, but
                    if (part.equalsIgnoreCase(event->String2)) { // Homie spec says it should be case-sensitive...
                      break;
                    }
                    i++;
                    part = parseStringKeepCase(enumList, i);
                  }
                  cmd += i;
                  cmd += ',';
                }
                cmd += event->String2;
              }
              handled = true;
            }
          }
          # endif // ifdef USES_P086
        }
      }
    }
  }

  if (handled) {
    MQTT_execute_command(cmd, tryRemoteConfig);
  }
  return handled;
}

/*****************************************************************************************
 * Execute commands received via MQTT, sanitize event arguments with regard to commas vs =
 * event/asyncevent are added to queue, other commands executed immediately
 ****************************************************************************************/
void MQTT_execute_command(String& cmd,
                          bool    tryRemoteConfig) {
  // in case of event, store to buffer and return...
  const String command = parseString(cmd, 1);

  if (equals(command, F("event")) || equals(command, F("asyncevent"))) {
    if (Settings.UseRules) {
      // Need to sanitize the event a bit to allow for sending event values as MQTT messages.
      // For example:
      // Publish topic: espeasy_node/cmd_arg2/event/myevent/2
      // Message: 1
      // Actual event:  myevent=1,2

      // Strip out the "event" or "asyncevent" part, leaving the actual event string
      String args = parseStringToEndKeepCase(cmd, 2);

      {
        // Get the first part upto a parameter separator
        // Example: "myEvent,1,2,3", which needs to be converted to "myEvent=1,2,3"
        // N.B. This may contain the first eventvalue too
        // e.g. "myEvent=1,2,3" => "myEvent=1"
        String eventName    = parseStringKeepCase(args, 1);
        String eventValues  = parseStringToEndKeepCase(args, 2);
        const int equal_pos = eventName.indexOf('=');

        if (equal_pos != -1) {
          // We found an '=' character, so the actual event name is everything before that char.
          eventName   = args.substring(0, equal_pos);
          eventValues = args.substring(equal_pos + 1); // Rest of the event, after the '=' char
        }

        if (eventValues.startsWith(F(","))) {
          // Need to reconstruct the event to get rid of calls like these:
          // myevent=,1,2
          eventValues = eventValues.substring(1);
        }

        // Now reconstruct the complete event
        // Without event values: "myEvent" (no '=' char)
        // With event values: "myEvent=1,2,3"

        // Re-using the 'cmd' String as that has pre-allocated memory which is
        // known to be large enough to hold the entire event.
        args = eventName;

        if (eventValues.length() > 0) {
          // Only append an = if there are eventvalues.
          args += '=';
          args += eventValues;
        }
      }

      // Check for duplicates, as sometimes a node may have multiple subscriptions to the same topic.
      // Then it may add several of the same events in a burst.
      eventQueue.addMove(std::move(args), true);
    }
  } else {
    ExecuteCommand(INVALID_TASK_INDEX, EventValueSource::Enum::VALUE_SOURCE_MQTT, cmd.c_str(), true, true, tryRemoteConfig);
  }
}

bool MQTT_protocol_send(EventStruct *event,
                        String       pubname,
                        bool         retainFlag) {
  bool success                = false;
  const bool contains_valname = pubname.indexOf(F("%valname%")) != -1;

  const uint8_t valueCount = getValueCountForTask(event->TaskIndex);

  for (uint8_t x = 0; x < valueCount; ++x) {
    // MFD: skip publishing for values with empty labels (removes unnecessary publishing of unwanted values)
    if (getTaskValueName(event->TaskIndex, x).isEmpty()) {
      continue; // we skip values with empty labels
    }
    String tmppubname = pubname;

    if (contains_valname) {
      parseSingleControllerVariable(tmppubname, event, x, false);
    }
    parseControllerVariables(tmppubname, event, false);
    String value;

    if (event->sensorType == Sensor_VType::SENSOR_TYPE_STRING) {
      value = event->String2.substring(0, 20); // For the log
    } else {
      value = formatUserVarNoCheck(event, x);
    }
    # ifndef BUILD_NO_DEBUG

    if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
      addLog(LOG_LEVEL_DEBUG, strformat(F("MQTT C%03d : %s %s"), event->ControllerIndex, tmppubname.c_str(), value.c_str()));
    }
    # endif // ifndef BUILD_NO_DEBUG

    // Small optimization so we don't try to copy potentially large strings
    if (event->sensorType == Sensor_VType::SENSOR_TYPE_STRING) {
      if (MQTTpublish(event->ControllerIndex, event->TaskIndex, tmppubname.c_str(), event->String2.c_str(),
                      retainFlag)) {
        success = true;
      }
    } else {
      // Publish using move operator, thus tmppubname and value are empty after this call
      if (MQTTpublish(event->ControllerIndex, event->TaskIndex, std::move(tmppubname), std::move(value),
                      retainFlag)) {
        success = true;
      }
    }
  }
  return success;
}

# if FEATURE_MQTT_DISCOVER

bool MQTT_SendAutoDiscovery(controllerIndex_t ControllerIndex, cpluginID_t CPluginID) {
  bool success = true;

  MakeControllerSettings(ControllerSettings); // -V522

  if (!AllocatedControllerSettings()) {
    return false;
  }
  LoadControllerSettings(ControllerIndex, *ControllerSettings);

  if (ControllerSettings->mqtt_autoDiscovery()

      // && (ControllerSettings->MqttAutoDiscoveryTrigger[0] != 0)
      && (ControllerSettings->MqttAutoDiscoveryTopic[0] != 0)) {
    #  ifndef BUILD_NO_DEBUG

    // FIXME change log level to DEBUG
    if (loglevelActiveFor(LOG_LEVEL_INFO)) {
      addLog(LOG_LEVEL_INFO, concat(strformat(F("MQTT : AutoDiscovery for Controller %d CPlugin %d"),
                                              ControllerIndex + 1, CPluginID),
                                    validTaskIndex(mqttDiscoverOnlyTask) ?
                                    strformat(F(", Task %d"), mqttDiscoverOnlyTask + 1) :
                                    EMPTY_STRING));
    }
    #  endif // ifndef BUILD_NO_DEBUG


    // Dispatch autoDiscovery per supported CPlugin
    switch (CPluginID) {
      case 5: // CPLUGIN_ID_005 : Home assistant/openHAB
        success = MQTT_HomeAssistant_SendAutoDiscovery(ControllerIndex, *ControllerSettings);
        break;
    }

    // FIXME Generate event when autoDiscovery started?
  }

  return success;
}

/********************************************************
 * Send MQTT AutoDiscovery in Home Assistant format
 *******************************************************/
bool MQTT_HomeAssistant_SendAutoDiscovery(controllerIndex_t         ControllerIndex,
                                          ControllerSettingsStruct& ControllerSettings) {
  bool success = false;

  // TODO Send global info?

  // Send plugin info
  taskIndex_t fromTask = 0;
  taskIndex_t maxTask  = TASKS_MAX;

  if (validTaskIndex(mqttDiscoverOnlyTask)) {
    fromTask             = mqttDiscoverOnlyTask;
    maxTask              = mqttDiscoverOnlyTask + 1;
    mqttDiscoverOnlyTask = INVALID_TASK_INDEX; // Reset
  }

  for (taskIndex_t x = fromTask; x < maxTask; ++x) {
    const pluginID_t pluginID = Settings.getPluginID_for_task(x);

    if (validPluginID_fullcheck(pluginID)) {
      LoadTaskSettings(x);
      const deviceIndex_t DeviceIndex = getDeviceIndex_from_TaskIndex(x);
      String discovery_topic(ControllerSettings.MqttAutoDiscoveryTopic);
      String publish_topic(ControllerSettings.Publish);
      String discoveryMessage;

      discoveryMessage.reserve(128);

      // Device is enabled so send information
      if (validDeviceIndex(DeviceIndex) &&
          Settings.TaskDeviceEnabled[x] &&                // task enabled?
          Settings.TaskDeviceSendData[ControllerIndex][x] // selected for this controller?
          ) {
        const String taskName   = getTaskDeviceName(x);
        const int    valueCount = getValueCountForTask(x);
        std::vector<DiscoveryItem> discoveryItems;

        #  ifndef BUILD_NO_DEBUG

        // FIXME change this log to DEBUG
        if (loglevelActiveFor(LOG_LEVEL_INFO)) {
          addLog(LOG_LEVEL_INFO, strformat(F("MQTT : Start AutoDiscovery for task %d, %s max. %d value%c"),
                                           x + 1, taskName.c_str(), valueCount, 1 == valueCount ? 's' : ' '));
        }
        #  endif // ifndef BUILD_NO_DEBUG

        // Translate Device[].VType to usable VType per value for MQTT AutoDiscovery
        if (MQTT_DiscoveryGetDeviceVType(x, discoveryItems, valueCount)) {
          const String hostName               = Settings.getHostname();
          const unsigned int groupId          = Settings.TaskDeviceID[ControllerIndex][x] & 0x3FFF; // Remove opt. flag bits
          const protocolIndex_t ProtocolIndex = getProtocolIndex_from_ControllerIndex(ControllerIndex);
          const bool usesControllerIDX        = validProtocolIndex(ProtocolIndex) &&
                                                getProtocolStruct(ProtocolIndex).usesID;

          const String elementName = groupId != 0 && !usesControllerIDX ?
                                     strformat(F("Group %u"), groupId) :
                                     strformat(F("%s %s"),    hostName.c_str(), taskName.c_str());
          const String elementIds = groupId != 0 && !usesControllerIDX ?
                                     strformat(F("group_%u"), groupId) :
                                     strformat(F("%s_%s"),    hostName.c_str(), taskName.c_str());

          // dev = device, ids = identifiers, mf = manufacturer, sw = sw_version
          const String deviceElement = strformat(F(",\"dev\":{\"name\":\"%s\",\"ids\":\"%s\","
                                                   "\"mf\":\"ESPEasy\",\"sw\":\"%s\"}"),
                                                 elementName.c_str(), elementIds.c_str(),
                                                 getSystemBuildString().c_str());

          for (size_t s = 0; s < discoveryItems.size(); ++s) {
            struct EventStruct TempEvent(x); // FIXME Check if this has enough data
            const uint8_t varCount = discoveryItems[s].varIndex + discoveryItems[s].valueCount;

            switch (discoveryItems[s].VType) {
              // VType values to support, mapped to device classes:
              case Sensor_VType::SENSOR_TYPE_SWITCH:

                for (uint8_t v = discoveryItems[s].varIndex; v < varCount; ++v) {
                  success &= MQTT_DiscoveryPublishWithStatusAndSet(x, v, taskName,
                                                                   ControllerIndex,
                                                                   publish_topic,
                                                                   discovery_topic,
                                                                   F("binary_sensor"),
                                                                   F("switch"),
                                                                   EMPTY_STRING, // No unit of measure
                                                                   &TempEvent,
                                                                   deviceElement,
                                                                   success,
                                                                   true, false);
                }
                break;
              case Sensor_VType::SENSOR_TYPE_TEMP_HUM:
              case Sensor_VType::SENSOR_TYPE_TEMP_BARO:
              case Sensor_VType::SENSOR_TYPE_TEMP_HUM_BARO:
              case Sensor_VType::SENSOR_TYPE_TEMP_EMPTY_BARO:
              case Sensor_VType::SENSOR_TYPE_TEMP_ONLY:
              case Sensor_VType::SENSOR_TYPE_HUM_ONLY:
              case Sensor_VType::SENSOR_TYPE_BARO_ONLY:
              {
                uint8_t v = discoveryItems[s].varIndex;

                // Temperature
                if (Sensor_VType::SENSOR_TYPE_HUM_ONLY != discoveryItems[s].VType) {
                  uint8_t fromV = v;
                  uint8_t maxV  = v + 1;

                  if (Sensor_VType::SENSOR_TYPE_TEMP_ONLY == discoveryItems[s].VType) {
                    maxV = fromV + discoveryItems[s].valueCount; // SENSOR_TYPE_TEMP_ONLY Can have multiple values
                  }

                  for (v = fromV; v < maxV; ++v) {
                    success &= MQTT_DiscoveryPublishWithStatusAndSet(x, v, taskName,
                                                                     ControllerIndex,
                                                                     publish_topic,
                                                                     discovery_topic,
                                                                     F("sensor"),
                                                                     F("temperature"),
                                                                     F("°C"),
                                                                     &TempEvent,
                                                                     deviceElement,
                                                                     success,
                                                                     false, false);
                  }
                }

                // Humidity
                if ((Sensor_VType::SENSOR_TYPE_TEMP_HUM == discoveryItems[s].VType) ||
                    (Sensor_VType::SENSOR_TYPE_TEMP_HUM_BARO == discoveryItems[s].VType) ||
                    (Sensor_VType::SENSOR_TYPE_HUM_ONLY == discoveryItems[s].VType)) {
                  uint8_t fromV = v;
                  uint8_t maxV  = v + 1;

                  if (Sensor_VType::SENSOR_TYPE_HUM_ONLY == discoveryItems[s].VType) {
                    maxV = fromV + discoveryItems[s].valueCount; // SENSOR_TYPE_HUM_ONLY Can have multiple values
                  }

                  for (v = fromV; v < maxV; ++v) {
                    success &= MQTT_DiscoveryPublishWithStatusAndSet(x, v, taskName,
                                                                     ControllerIndex,
                                                                     publish_topic,
                                                                     discovery_topic,
                                                                     F("sensor"),
                                                                     F("humidity"),
                                                                     F("%"),
                                                                     &TempEvent,
                                                                     deviceElement,
                                                                     success,
                                                                     false, false);
                  }
                }

                // Barometric pressure
                if ((Sensor_VType::SENSOR_TYPE_TEMP_BARO == discoveryItems[s].VType) ||
                    (Sensor_VType::SENSOR_TYPE_TEMP_HUM_BARO == discoveryItems[s].VType) ||
                    (Sensor_VType::SENSOR_TYPE_TEMP_EMPTY_BARO == discoveryItems[s].VType) ||
                    (Sensor_VType::SENSOR_TYPE_BARO_ONLY == discoveryItems[s].VType)) {
                  if (Sensor_VType::SENSOR_TYPE_TEMP_EMPTY_BARO == discoveryItems[s].VType) {
                    v++; // Skip 2nd value = 'EMPTY'
                  }
                  success &= MQTT_DiscoveryPublishWithStatusAndSet(x, v, taskName,
                                                                   ControllerIndex,
                                                                   publish_topic,
                                                                   discovery_topic,
                                                                   F("sensor"),
                                                                   F("pressure"),
                                                                   F("hPa"),
                                                                   &TempEvent,
                                                                   deviceElement,
                                                                   success,
                                                                   false, false);
                  v++;
                }
                break;
              }
              case Sensor_VType::SENSOR_TYPE_DISTANCE_ONLY:

                for (uint8_t v = discoveryItems[s].varIndex; v < varCount; ++v) {
                  success &= MQTT_DiscoveryPublishWithStatusAndSet(x, v, taskName,
                                                                   ControllerIndex,
                                                                   publish_topic,
                                                                   discovery_topic,
                                                                   F("sensor"),
                                                                   F("distance"),
                                                                   F("cm"),
                                                                   &TempEvent,
                                                                   deviceElement,
                                                                   success,
                                                                   false, false);
                }
                break;

              case Sensor_VType::SENSOR_TYPE_DUSTPM2_5_ONLY:
              case Sensor_VType::SENSOR_TYPE_DUSTPM1_0_ONLY:
              case Sensor_VType::SENSOR_TYPE_DUSTPM10_ONLY:
              {
                const __FlashStringHelper*dev = (Sensor_VType::SENSOR_TYPE_DUSTPM2_5_ONLY == discoveryItems[s].VType ? F("pm25") :
                                                 Sensor_VType::SENSOR_TYPE_DUSTPM1_0_ONLY == discoveryItems[s].VType ? F("pm1") :
                                                 F("pm10"));

                for (uint8_t v = discoveryItems[s].varIndex; v < varCount; ++v) {
                  success &= MQTT_DiscoveryPublishWithStatusAndSet(x, v, taskName,
                                                                   ControllerIndex,
                                                                   publish_topic,
                                                                   discovery_topic,
                                                                   F("sensor"),
                                                                   dev,
                                                                   F("µg/m³"),
                                                                   &TempEvent,
                                                                   deviceElement,
                                                                   success,
                                                                   false, false);
                }
                break;
              }
              case Sensor_VType::SENSOR_TYPE_CO2_ONLY:
              case Sensor_VType::SENSOR_TYPE_TVOC_ONLY:
              {
                const __FlashStringHelper*dev = Sensor_VType::SENSOR_TYPE_CO2_ONLY == discoveryItems[s].VType ?
                                                F("carbon_dioxide") : F("volatile_organic_compounds");
                const __FlashStringHelper*uom = Sensor_VType::SENSOR_TYPE_CO2_ONLY == discoveryItems[s].VType ?
                                                F("ppm") : F("ppd");

                for (uint8_t v = discoveryItems[s].varIndex; v < varCount; ++v) {
                  success &= MQTT_DiscoveryPublishWithStatusAndSet(x, v, taskName,
                                                                   ControllerIndex,
                                                                   publish_topic,
                                                                   discovery_topic,
                                                                   F("sensor"),
                                                                   dev,
                                                                   uom,
                                                                   &TempEvent,
                                                                   deviceElement,
                                                                   success,
                                                                   false, false);
                }
                break;
              }
              case Sensor_VType::SENSOR_TYPE_WEIGHT_ONLY:

                for (uint8_t v = discoveryItems[s].varIndex; v < varCount; ++v) {
                  success &= MQTT_DiscoveryPublishWithStatusAndSet(x, v, taskName,
                                                                   ControllerIndex,
                                                                   publish_topic,
                                                                   discovery_topic,
                                                                   F("sensor"),
                                                                   F("mdi:scale"), // Icon
                                                                   F("kg"),
                                                                   &TempEvent,
                                                                   deviceElement,
                                                                   success,
                                                                   false, true);
                }
                break;
              case Sensor_VType::SENSOR_TYPE_MOISTURE_ONLY:

                for (uint8_t v = discoveryItems[s].varIndex; v < varCount; ++v) {
                  success &= MQTT_DiscoveryPublishWithStatusAndSet(x, v, taskName,
                                                                   ControllerIndex,
                                                                   publish_topic,
                                                                   discovery_topic,
                                                                   F("sensor"),
                                                                   F("mdi:cup-water"), // Icon
                                                                   F("%"),
                                                                   &TempEvent,
                                                                   deviceElement,
                                                                   success,
                                                                   false, true);
                }
                break;
              case Sensor_VType::SENSOR_TYPE_VOLTAGE_ONLY:
              case Sensor_VType::SENSOR_TYPE_CURRENT_ONLY:
              {
                const __FlashStringHelper*dev = Sensor_VType::SENSOR_TYPE_VOLTAGE_ONLY == discoveryItems[s].VType ?
                                                F("voltage") : F("current");
                const __FlashStringHelper*uom = Sensor_VType::SENSOR_TYPE_VOLTAGE_ONLY == discoveryItems[s].VType ?
                                                F("V") : F("A");

                for (uint8_t v = discoveryItems[s].varIndex; v < varCount; ++v) {
                  success &= MQTT_DiscoveryPublishWithStatusAndSet(x, v, taskName,
                                                                   ControllerIndex,
                                                                   publish_topic,
                                                                   discovery_topic,
                                                                   F("sensor"),
                                                                   dev,
                                                                   uom,
                                                                   &TempEvent,
                                                                   deviceElement,
                                                                   success,
                                                                   false, false);
                }
                break;
              }
              case Sensor_VType::SENSOR_TYPE_POWER_FACT_ONLY:
              case Sensor_VType::SENSOR_TYPE_POWER_USG_ONLY:
              case Sensor_VType::SENSOR_TYPE_APPRNT_POWER_USG_ONLY:
              {
                const __FlashStringHelper*dev = Sensor_VType::SENSOR_TYPE_POWER_FACT_ONLY == discoveryItems[s].VType ?
                                                F("power_factor") : F("power");
                const __FlashStringHelper*uom = Sensor_VType::SENSOR_TYPE_POWER_FACT_ONLY == discoveryItems[s].VType ? F("Cos φ") :
                                                Sensor_VType::SENSOR_TYPE_POWER_USG_ONLY == discoveryItems[s].VType ? F("kWh") :
                                                F("VA");

                for (uint8_t v = discoveryItems[s].varIndex; v < varCount; ++v) {
                  success &= MQTT_DiscoveryPublishWithStatusAndSet(x, v, taskName,
                                                                   ControllerIndex,
                                                                   publish_topic,
                                                                   discovery_topic,
                                                                   F("sensor"),
                                                                   dev,
                                                                   uom,
                                                                   &TempEvent,
                                                                   deviceElement,
                                                                   success,
                                                                   false, false);
                }
                break;
              }
              case Sensor_VType::SENSOR_TYPE_LUX_ONLY:

                for (uint8_t v = discoveryItems[s].varIndex; v < varCount; ++v) {
                  success &= MQTT_DiscoveryPublishWithStatusAndSet(x, v, taskName,
                                                                   ControllerIndex,
                                                                   publish_topic,
                                                                   discovery_topic,
                                                                   F("sensor"),
                                                                   F("illuminance"),
                                                                   F("lux"),
                                                                   &TempEvent,
                                                                   deviceElement,
                                                                   success,
                                                                   false, false);
                }
                break;

              case Sensor_VType::SENSOR_TYPE_COLOR_RED_ONLY:
              case Sensor_VType::SENSOR_TYPE_COLOR_GREEN_ONLY:
              case Sensor_VType::SENSOR_TYPE_COLOR_BLUE_ONLY:
              {
                const __FlashStringHelper*uom = Sensor_VType::SENSOR_TYPE_COLOR_RED_ONLY == discoveryItems[s].VType ? F("R") :
                                                Sensor_VType::SENSOR_TYPE_COLOR_GREEN_ONLY == discoveryItems[s].VType ? F("G") :
                                                F("B");

                for (uint8_t v = discoveryItems[s].varIndex; v < varCount; ++v) {
                  success &= MQTT_DiscoveryPublishWithStatusAndSet(x, v, taskName,
                                                                   ControllerIndex,
                                                                   publish_topic,
                                                                   discovery_topic,
                                                                   F("sensor"),
                                                                   F("mdi:palette"),
                                                                   uom,
                                                                   &TempEvent,
                                                                   deviceElement,
                                                                   success,
                                                                   false, true);
                }
                break;
              }
              case Sensor_VType::SENSOR_TYPE_COLOR_TEMP_ONLY:

                for (uint8_t v = discoveryItems[s].varIndex; v < varCount; ++v) {
                  success &= MQTT_DiscoveryPublishWithStatusAndSet(x, v, taskName,
                                                                   ControllerIndex,
                                                                   publish_topic,
                                                                   discovery_topic,
                                                                   F("sensor"),
                                                                   F("mdi:temperature-kelvin"),
                                                                   F("K"),
                                                                   &TempEvent,
                                                                   deviceElement,
                                                                   success,
                                                                   false, true);
                }
                break;

              case Sensor_VType::SENSOR_TYPE_WIND:
              case Sensor_VType::SENSOR_TYPE_ANALOG_ONLY:
              case Sensor_VType::SENSOR_TYPE_DIRECTION_ONLY:
              case Sensor_VType::SENSOR_TYPE_GPS_ONLY:
              case Sensor_VType::SENSOR_TYPE_UV_ONLY:
              case Sensor_VType::SENSOR_TYPE_UV_INDEX_ONLY:
              case Sensor_VType::SENSOR_TYPE_IR_ONLY:
                // TODO
                break;

              // VType values to ignore, will/should be mapped into something more explicit
              case Sensor_VType::SENSOR_TYPE_NONE:
              case Sensor_VType::SENSOR_TYPE_SINGLE:
              case Sensor_VType::SENSOR_TYPE_DUAL:
              case Sensor_VType::SENSOR_TYPE_TRIPLE:
              case Sensor_VType::SENSOR_TYPE_QUAD:
              case Sensor_VType::SENSOR_TYPE_DIMMER: // Maybe implement?
              case Sensor_VType::SENSOR_TYPE_STRING:
              case Sensor_VType::SENSOR_TYPE_ULONG:
              #  if FEATURE_EXTENDED_TASK_VALUE_TYPES
              case Sensor_VType::SENSOR_TYPE_UINT32_DUAL:
              case Sensor_VType::SENSOR_TYPE_UINT32_TRIPLE:
              case Sensor_VType::SENSOR_TYPE_UINT32_QUAD:
              case Sensor_VType::SENSOR_TYPE_INT32_SINGLE:
              case Sensor_VType::SENSOR_TYPE_INT32_DUAL:
              case Sensor_VType::SENSOR_TYPE_INT32_TRIPLE:
              case Sensor_VType::SENSOR_TYPE_INT32_QUAD:
              case Sensor_VType::SENSOR_TYPE_UINT64_SINGLE:
              case Sensor_VType::SENSOR_TYPE_UINT64_DUAL:
              case Sensor_VType::SENSOR_TYPE_INT64_SINGLE:
              case Sensor_VType::SENSOR_TYPE_INT64_DUAL:
              case Sensor_VType::SENSOR_TYPE_DOUBLE_SINGLE:
              case Sensor_VType::SENSOR_TYPE_DOUBLE_DUAL:
              #  endif // if FEATURE_EXTENDED_TASK_VALUE_TYPES
              case Sensor_VType::SENSOR_TYPE_NOT_SET:
                break;
            }
          }
        }
      }
    }
  }

  return success;
}

bool MQTT_DiscoveryGetDeviceVType(taskIndex_t                 TaskIndex,
                                  std::vector<DiscoveryItem>& discoveryItems,
                                  int                         valueCount) {
  const deviceIndex_t DeviceIndex = getDeviceIndex_from_TaskIndex(TaskIndex);
  const size_t orgLen             = discoveryItems.size();
  Sensor_VType VType              = Device[DeviceIndex].VType;

  // For those plugin IDs that don't have an explicitly set VType, or the wrong VType set
  // Deliberately ignored/skipped for now:
  // case 3:   // Pulse: Needs special handling/not usable?
  // case 8:   // RFID Wiegand
  // case 17:  // RFID PN532
  // case 33:  // Dummy: Needs special handling/not usable?
  // case 40:  // RFID ID12
  // case 58:  // Keypad: Not a switch
  // case 59:  // Encoder: Not a switch
  // case 61:  // Keypad: Not a switch
  // case 62:  // Touch Keypad: Not a switch
  // case 63:  // Touch Keypad: Not a switch
  // case 111: // RFID RC522

  // TODO: To be reviewed/considered later/custom/special handled:
  // case 26:  // Sysinfo: Needs special handling/not usable?
  // case 45:  // MPU6050
  // case 50:  // TCS34725 RGB: Needs special handling
  // case 64:  // APDS9960: Needs special handling
  // case 66:  // VEML6040 RGB: Needs special handling
  // case 71:  // Kamstrup Heat
  // case 76:  // HLW8012: Needs special handling
  // case 77:  // CSE7766: Needs special handling
  // case 78:  // Eastron: Needs special handling
  // case 82:  // GPS: Needs special handling
  // case 85:  // AcuDC243: Needs special handling
  // case 92:  // DL-bus: Needs special handling
  // case 93:  // Mitsubishi Heatpump
  // case 102: // PZEM004: Needs special handling
  // case 103: // Atlas EZO: Needs special handling
  // case 108: // DDS238: Needs special handling
  // case 112: // AS7265x: Needs special handling
  // case 115: // MAX1704x
  // case 117: // SCD30 CO2: Needs special handling
  // case 132: // INA3221: Needs special handling
  // case 142: // AS5600 Angle/rotation
  // case 145: // MQxxx gases
  // case 159: // LD2410
  // case 163: // RadSens
  // case 167: // Vindstyrka: Needs special handling
  // case 169: // AS3935 Lightning
  // case 176: // Victron VE.Direct: Needs user-configurable Sensor_VType + Unit_of_measure per value

  // Plugin ID numbers not listed in the above switch statement either have the correct Sensor_VType value set in DeviceStruct,
  // or have PLUGIN_GET_DISCOVERY_VTYPES implemented

  struct EventStruct TempEvent(TaskIndex);
  String dummy;

  TempEvent.Par5 = valueCount; // Pass in the provided value count, so we don't have to determine that again

  // Get value VTypes from plugin
  if (PluginCall(PLUGIN_GET_DISCOVERY_VTYPES, &TempEvent, dummy)) {
    #  ifndef BUILD_NO_DEBUG

    // FIXME change this log to DEBUG level
    if (loglevelActiveFor(LOG_LEVEL_INFO)) {
      addLog(LOG_LEVEL_INFO, strformat(F("MQTT : AutoDiscovery using Plugin-defined config (%d)"), discoveryItems.size()));
    }
    #  endif // ifndef BUILD_NO_DEBUG

    uint8_t maxVar = VARS_PER_TASK;

    for (; maxVar > 0; --maxVar) { // Only include minimal used values
      if (TempEvent.ParN[maxVar - 1] != 0) {
        break;
      }
    }

    for (uint8_t v = 0; v < maxVar; ++v) {
      const Sensor_VType VType = static_cast<Sensor_VType>(TempEvent.ParN[v]);

      if (Sensor_VType::SENSOR_TYPE_NONE != VType) {
        discoveryItems.push_back(DiscoveryItem(VType, 1, v));
      }
    }

    #  ifndef BUILD_NO_DEBUG

    // FIXME change this log to DEBUG level
    if (loglevelActiveFor(LOG_LEVEL_INFO)) {
      addLog(LOG_LEVEL_INFO, strformat(F("MQTT : AutoDiscovery Plugin specific config added (%d)"), discoveryItems.size()));
    }
    #  endif // ifndef BUILD_NO_DEBUG
  } else {
    // Use Device VType setting
    if (Sensor_VType::SENSOR_TYPE_NONE != VType) {
      discoveryItems.push_back(DiscoveryItem(VType, valueCount, 0));

      #  ifndef BUILD_NO_DEBUG

      // FIXME change this log to DEBUG level
      if (loglevelActiveFor(LOG_LEVEL_INFO)) {
        addLog(LOG_LEVEL_INFO, strformat(F("MQTT : AutoDiscovery Default Plugin config (%d)"), discoveryItems.size()));
      }
      #  endif // ifndef BUILD_NO_DEBUG
    }
  }

  return discoveryItems.size() != orgLen; // Something added?
}

String MQTT_TaskValueUniqueName(const String& taskName,
                                const String& valueName) {
  String uniqueId = strformat(F("%s_%s_%s"), Settings.getHostname().c_str(), taskName.c_str(), valueName.c_str());

  uniqueId.toLowerCase();
  return uniqueId;
}

String MQTT_DiscoveryBuildValueTopic(const String            & topic,
                                     struct EventStruct       *event,
                                     uint8_t                   taskValueIndex,
                                     const __FlashStringHelper*deviceClass) {
  String tmpTopic(topic);

  parseSingleControllerVariable(tmpTopic, event, taskValueIndex, false);
  parseDeviceClassVariable(tmpTopic, deviceClass, false);
  parseControllerVariables(tmpTopic, event, false); // Replace this last

  while (tmpTopic.endsWith(F("/"))) {
    tmpTopic = tmpTopic.substring(0, tmpTopic.length() - 1);
  }

  return tmpTopic;
}

bool MQTT_DiscoveryPublish(controllerIndex_t ControllerIndex,
                           const String    & topic,
                           const String    & discoveryMessage,
                           taskIndex_t       x,
                           uint8_t           v,
                           const String    & taskName) {
  bool result = false;

  // Send each discovery message separate
  result = MQTTpublish(ControllerIndex, INVALID_TASK_INDEX, concat(topic, F("/config")).c_str(), discoveryMessage.c_str(), false);
  #  ifndef BUILD_NO_DEBUG

  // FIXME change this log to DEBUG level
  if (loglevelActiveFor(LOG_LEVEL_INFO)) {
    addLog(LOG_LEVEL_INFO, strformat(F("MQTT : Discovery %d Task %d '%s': disc.topic: %s"),
                                     result, x + 1, taskName.c_str(), concat(topic, F("/config")).c_str()));
    addLog(LOG_LEVEL_INFO, strformat(F("MQTT : Discovery payload: %s"),
                                     discoveryMessage.c_str()));
  }
  #  endif // ifndef BUILD_NO_DEBUG
  return result;
}

bool MQTT_DiscoveryPublishWithStatusAndSet(taskIndex_t               taskIndex,
                                           uint8_t                   taskValue,
                                           String                    taskName,
                                           controllerIndex_t         ControllerIndex,
                                           String                    publishTopic,
                                           String                    discoveryTopic,
                                           const __FlashStringHelper*componentClass,
                                           String                    deviceClass,
                                           String                    unitOfMeasure,
                                           struct EventStruct       *event,
                                           const String              deviceElement,
                                           bool                      success,
                                           bool                      hasSet,
                                           bool                      hasIcon) {
  const String withSet   = hasSet ? F(",\"cmd_t\":\"~/set\"") : EMPTY_STRING;
  const String devOrIcon = hasIcon ? F("ic") : F("dev_cla");
  const String withUoM   = !unitOfMeasure.isEmpty() ?
                           strformat(F(",\"unit_of_meas\":\"%s\""), unitOfMeasure.c_str()) :
                           EMPTY_STRING;
  const String valuename = getTaskValueName(taskIndex, taskValue);

  if (!valuename.isEmpty()) {
    const String uniqueId         = MQTT_TaskValueUniqueName(taskName, valuename);
    const String publish          = MQTT_DiscoveryBuildValueTopic(publishTopic, event, taskValue, componentClass);
    const String discovery        = MQTT_DiscoveryBuildValueTopic(discoveryTopic, event, taskValue, componentClass);
    const String discoveryMessage = strformat(F("{\"~\":\"%s\",\"name\":\"%s %s\",\"uniq_id\":\"%s\",\"schema\":\"basic\","
                                                "\"%s\":\"%s\"%s%s,\"stat_t\":\"~\""
                                                "%s}"), // deviceElement last
                                              publish.c_str(), taskName.c_str(), valuename.c_str(), uniqueId.c_str(),
                                              devOrIcon.c_str(), deviceClass.c_str(), withUoM.c_str(), withSet.c_str(),
                                              deviceElement.c_str());

    return MQTT_DiscoveryPublish(ControllerIndex, discovery, discoveryMessage, taskIndex, taskValue, taskName);
  }
  return success;
}

# endif // if FEATURE_MQTT_DISCOVER

#endif // if FEATURE_MQTT
