#include "../PluginStructs/P180_data_struct.h"

#ifdef USES_P180
# include <GPIO_Direct_Access.h>
# include "../Globals/RulesCalculate.h"

P180_data_struct::P180_data_struct(struct EventStruct *event) {
  _enPin        = P180_ENABLE_PIN;
  _rstPin       = P180_RST_PIN;
  _showLog      = P180_LOG_DEBUG == 1;
  _taskIndex    = event->TaskIndex;
  busCmd_Helper = new (std::nothrow) BusCmd_Helper_struct(_taskIndex,
                                                          _enPin,
                                                          _rstPin,
                                                          P180_NR_OUTPUT_VALUES);

  if (nullptr != busCmd_Helper) {
    busCmd_Helper->setLog(_showLog);
    busCmd_Helper->setI2CAddress(P180_I2C_ADDRESS); // FIXME

    loadStrings(event);

    if (_stringsLoaded) {
      for (uint8_t i = 0; i < VARS_PER_TASK; ++i) {
        busCmd_Helper->setBuffer(i, _strings[P180_BUFFER_START_CACHE + i], _strings[P180_BUFFER_START_COMMANDS + i]);
        _strings[P180_BUFFER_START_CACHE + i].clear(); // Clear memory
        _strings[P180_BUFFER_START_COMMANDS + i].clear();
      }
    }
  }

  # if defined(FEATURE_MQTT_DISCOVER) && FEATURE_MQTT_DISCOVER // When feature is available

  for (uint8_t i = 0; i < VARS_PER_TASK; ++i) {
    if (i < P180_NR_OUTPUT_VALUES) {
      _vTypes[i] = static_cast<Sensor_VType>(PCONFIG(P180_VALUE_OFFSET + i));
    } else {
      _vTypes[i] = Sensor_VType::SENSOR_TYPE_SINGLE;
    }
  }
  # endif // if defined(FEATURE_MQTT_DISCOVER) && FEATURE_MQTT_DISCOVER
}

P180_data_struct::~P180_data_struct() {
  _strings->clear();
  # if defined(FEATURE_MQTT_DISCOVER) && FEATURE_MQTT_DISCOVER // When feature is available
  _vTypes.clear();
  # endif // if defined(FEATURE_MQTT_DISCOVER) && FEATURE_MQTT_DISCOVER
}

bool P180_data_struct::plugin_init(struct EventStruct *event) {
  if (validGpio(_enPin)) {
    pinMode(_enPin, OUTPUT);
    DIRECT_pinWrite(_enPin, HIGH);
  }

  if (validGpio(_rstPin)) {
    pinMode(_rstPin, OUTPUT);
    DIRECT_pinWrite(_rstPin, HIGH);
  }

  loadStrings(event);

  if (nullptr != busCmd_Helper) {
    _initialized = true;

    if (_stringsLoaded && !_strings[P180_BUFFER_ENTRY_INIT].isEmpty()) {
      _initialized = busCmd_Helper->parseAndExecute(BusCmd_CommandSource_e::PluginRead, // To allow logging
                                                    _strings[P180_BUFFER_ENTRY_INIT],
                                                    F("P180 : Device initialization with %d commands."));
    }
  }

  return _initialized;
}

void P180_data_struct::plugin_exit(struct EventStruct *event) {
  loadStrings(event);

  if (_stringsLoaded && !_strings[P180_BUFFER_ENTRY_EXIT].isEmpty() && (nullptr != busCmd_Helper)) {
    busCmd_Helper->parseAndExecute(BusCmd_CommandSource_e::PluginRead, // To allow logging
                                   _strings[P180_BUFFER_ENTRY_EXIT],
                                   F("P180 : Device shutdown/exit with %d commands."));
  }
}

void P180_data_struct::loadStrings(struct EventStruct*event) {
  if (!_stringsLoaded) {
    LoadCustomTaskSettings(event->TaskIndex, _strings, P180_CUSTOM_BUFFER_SIZE, 0);
    _stringsLoaded = true;
  }
}

bool P180_data_struct::plugin_once_a_second(struct EventStruct *event) {
  if (nullptr != busCmd_Helper) {
    return busCmd_Helper->plugin_once_a_second(event);
  }
  return false;
}

bool P180_data_struct::plugin_ten_per_second(struct EventStruct *event) {
  if (nullptr != busCmd_Helper) {
    return busCmd_Helper->plugin_ten_per_second(event);
  }
  return false;
}

bool P180_data_struct::plugin_fifty_per_second(struct EventStruct *event) {
  if (nullptr != busCmd_Helper) {
    return busCmd_Helper->plugin_fifty_per_second(event);
  }
  return false;
}

bool P180_data_struct::plugin_read(struct EventStruct *event) {
  if (nullptr != busCmd_Helper) {
    return busCmd_Helper->plugin_read(event);
  }
  return false;
}

/*********************************************************************************************
 * Handle command processing
 ********************************************************************************************/
bool P180_data_struct::plugin_write(struct EventStruct *event,
                                    String            & string) {
  bool success     = false;
  const String cmd = parseString(string, 1);

  if (equals(cmd, F("geni2c"))) {
    const String sub       = parseString(string, 2);
    const String par3      = parseStringKeepCaseNoTrim(string, 3);
    const String varName   = parseString(string, 4);
    const bool   hasPar3   = !par3.isEmpty();
    const bool   hasPar4   = !varName.isEmpty();
    const bool   hasPar5   = !parseString(string, 5).isEmpty();
    const bool   hasBusCmd = nullptr != busCmd_Helper;
    std::vector<BusCmd_Command_struct> cmds;
    taskVarIndex_t taskVar = INVALID_TASKVAR_INDEX;
    uint32_t val{};

    if (hasPar4) {
      if (validUIntFromString(varName, val) && (val > 0) && (val <= VARS_PER_TASK)) {
        taskVar = val - 1;
      } else {
        taskVar = findDeviceValueIndexByName(varName, event->TaskIndex);
      }
    }

    if (equals(sub, F("cmd")) && hasPar3 && hasBusCmd) { // genI2c,cmd,<I2Ccommands>[,<TaskValueIndex>[,<cache_name>]]
      String cacheName;

      if (hasPar5) {
        cacheName = parseString(string, 5);
      }

      cmds = busCmd_Helper->parseBusCmdCommands(cacheName, par3, !cacheName.isEmpty());
    } else if (equals(sub, F("exec")) && hasPar3 && hasBusCmd) {     // genI2c,exec,<cache_name>[,<TaskValueIndex>]
      cmds = busCmd_Helper->parseBusCmdCommands(par3, EMPTY_STRING); // Fetch commands from cache by name
    } else if (equals(sub, F("log")) && hasPar3) {                   // genI2c,log,<1|0>
      _showLog       = event->Par3 != 0;
      P180_LOG_DEBUG = _showLog ? 1 : 0;

      if (hasBusCmd) {
        busCmd_Helper->setLog(_showLog);
      }
      success = true;
    }

    if (!cmds.empty() && hasBusCmd && (BusCmd_CommandState_e::Idle == busCmd_Helper->getCommandState())) { // Execute commands when not busy
      busCmd_Helper->setCommands(cmds,
                                 taskVar,
                                 0,
                                 1, // Process single entry
                                 BusCmd_CommandState_e::Processing);
      Scheduler.schedule_task_device_timer(_taskIndex, millis() + 5);

      success = true;
    }
  }

  return success;
}

#endif // ifdef USES_P180
