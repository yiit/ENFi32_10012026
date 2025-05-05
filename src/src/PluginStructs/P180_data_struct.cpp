#include "../PluginStructs/P180_data_struct.h"

#ifdef USES_P180
# include <GPIO_Direct_Access.h>
# include "../Globals/RulesCalculate.h"

// These commands (not case-sensitive) must have the same order as the P180_Commands_e enum class
const char P180_commands[] PROGMEM =
  "n|g|p|r|w|s|t|e|c|v|d|a|z|i|l|";
const char P180_commandsLong[] PROGMEM =
  "nop|get|put|read|write|read16|write16|eval|calc|value|delay|enable|reset|if|let|";

// Supported data formats, _ == undefined, not processed
const char P180_dataFormats[] PROGMEM =
  "_|"
  "u8|"
  "u16|"
  "u24|"
  "u32|"
  "u16le|"
  "u24le|"
  "u32le|"
  "8|"
  "16|"
  "24|"
  "32|"
  "16le|"
  "24le|"
  "32le|"
  "b|"
  "w|"
;

P180_Command_struct::P180_Command_struct(P180_Command_e    _command,
                                         P180_DataFormat_e _format,
                                         uint16_t          _reg,
                                         int64_t           _data,
                                         uint32_t          _len,
                                         String            _calculation,
                                         String            _variable)
  :command(_command), format(_format), reg(_reg), len(_len), calculation(_calculation), variable(_variable) {
  switch (format) {
    case P180_DataFormat_e::undefined: d0_uint32_t = static_cast<uint32_t>(_data); break; // Special case
    case P180_DataFormat_e::uint8_t: d0_uint8_t    = static_cast<uint8_t>(_data); break;
    case P180_DataFormat_e::uint16_t:
    case P180_DataFormat_e::uint16_t_LE: d0_uint16_t = static_cast<uint16_t>(_data); break;
    case P180_DataFormat_e::uint24_t:
    case P180_DataFormat_e::uint32_t:
    case P180_DataFormat_e::uint24_t_LE:
    case P180_DataFormat_e::uint32_t_LE: d0_uint32_t = static_cast<uint32_t>(_data); break;
    case P180_DataFormat_e::int8_t: d0_int8_t        = _data; break;
    case P180_DataFormat_e::int16_t:
    case P180_DataFormat_e::int16_t_LE: d0_int16_t = _data; break;
    case P180_DataFormat_e::int24_t:
    case P180_DataFormat_e::int32_t:
    case P180_DataFormat_e::int24_t_LE:
    case P180_DataFormat_e::int32_t_LE: d0_int32_t = _data; break;
    case P180_DataFormat_e::bytes:
    case P180_DataFormat_e::words: break;
  }
}

P180_Command_struct::~P180_Command_struct() {
  data_b.clear();
  data_w.clear();
  calculation.clear();
}

int64_t P180_Command_struct::getIntValue() {
  int64_t data{};

  switch (format) {
    case P180_DataFormat_e::undefined: break;
    case P180_DataFormat_e::uint8_t: data = d0_uint8_t; break;
    case P180_DataFormat_e::uint16_t:
    case P180_DataFormat_e::uint16_t_LE: data = d0_uint16_t; break;
    case P180_DataFormat_e::uint24_t:
    case P180_DataFormat_e::uint32_t:
    case P180_DataFormat_e::uint24_t_LE:
    case P180_DataFormat_e::uint32_t_LE: data = d0_uint32_t; break;
    case P180_DataFormat_e::int8_t: data      = d0_int8_t; break;
    case P180_DataFormat_e::int16_t:
    case P180_DataFormat_e::int16_t_LE: data = d0_int16_t; break;
    case P180_DataFormat_e::int24_t:
    case P180_DataFormat_e::int32_t:
    case P180_DataFormat_e::int24_t_LE:
    case P180_DataFormat_e::int32_t_LE: data = d0_int32_t; break;
    case P180_DataFormat_e::bytes:
    case P180_DataFormat_e::words: break;
  }
  return data;
}

String P180_Command_struct::getHexValue(const bool withPrefix) {
  return withPrefix ? concat(F("0x"), getHexValue()) : getHexValue();
}

String P180_Command_struct::getHexValue() {
  uint64_t data64 = getUIntValue();

  if (P180_DataFormat_e::bytes == format) {
    return formatToHex_array(&data_b[0], data_b.size());
  } else if (P180_DataFormat_e::words == format) {
    return formatToHex_wordarray(&data_w[0], data_w.size());
  } else {
    return formatToHex(data64);
  }
}

# ifndef LIMIT_BUILD_SIZE
String P180_Command_struct::toString() {
  char cmd[10]{};
  char cmdS[3]{};
  char fmt[10]{};

  GetTextIndexed(cmd,  sizeof(cmd),  static_cast<uint32_t>(command), P180_commandsLong);
  GetTextIndexed(cmdS, sizeof(cmdS), static_cast<uint32_t>(command), P180_commands);
  GetTextIndexed(fmt,  sizeof(fmt),  static_cast<uint32_t>(format),  P180_dataFormats);
  String data;

  if ((P180_DataFormat_e::bytes == format) || (P180_DataFormat_e::words == format)) {
    data = getHexValue(true);
  }

  int64_t val{};

  if ((P180_Command_e::Delay == command) || (P180_Command_e::Value == command)) {
    val = d0_uint32_t;
  } else {
    val = getIntValue();
  }
  String result = strformat(F("cmd: '%s' (%s), fmt: '%s', reg: 0x%x, data: %d (0x%llx), len: %d"),
                            cmd, cmdS, fmt, reg, val, val, len);

  if (!data.isEmpty()) {
    result = concat(result, strformat(F(", data_b/w: %s"), data.c_str()));
  }

  if (!variable.isEmpty()) {
    result = concat(result, strformat(F(", variable: %s"), variable.c_str()));
  }

  if (!calculation.isEmpty()) {
    result = concat(result, strformat(F(", calculation: %s"), calculation.c_str()));
  }
  return result;
}

# endif // ifndef LIMIT_BUILD_SIZE

const __FlashStringHelper * P180_data_struct::cacheSuffix(P180_CommandSource_e source) {
  switch (source) {
    case P180_CommandSource_e::PluginIdle:
    case P180_CommandSource_e::PluginRead: return F("");
    case P180_CommandSource_e::PluginOncePerSecond: return F("_1ps");
    case P180_CommandSource_e::PluginTenPerSecond: return F("_10ps");
    case P180_CommandSource_e::PluginFiftyPerSecond: return F("_50ps");
  }
  return F("");
}

P180_data_struct::P180_data_struct(struct EventStruct *event) {
  _taskIndex  = event->TaskIndex;
  _i2cAddress = P180_I2C_ADDRESS;
  _enPin      = P180_ENABLE_PIN;
  _rstPin     = P180_RST_PIN;
  _showLog    = P180_LOG_DEBUG == 1;

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
  _commandCache.clear();
  _commands.clear();
  _strings->clear();
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

  _initialized = true;

  if (_stringsLoaded && !_strings[P180_BUFFER_ENTRY_INIT].isEmpty()) {
    _commandSource = P180_CommandSource_e::PluginRead; // To allow logging
    _commands      = parseI2CCommands(EMPTY_STRING, _strings[P180_BUFFER_ENTRY_INIT]);

    if (!_commands.empty()) {
      if (loglevelActiveFor(LOG_LEVEL_INFO) && _showLog) {
        addLog(LOG_LEVEL_INFO, strformat(F("P180 : Device initialization with %d commands."), _commands.size()));
      }
      _commandState = P180_CommandState_e::Processing;
      _initialized  = executeI2CCommands();
      _commands.clear();
      _commandState = P180_CommandState_e::Idle;
    }
    _commandSource = P180_CommandSource_e::PluginIdle;
  }

  return _initialized;
}

void P180_data_struct::plugin_exit(struct EventStruct *event) {
  loadStrings(event);

  if (_stringsLoaded && !_strings[P180_BUFFER_ENTRY_EXIT].isEmpty()) {
    _commandSource = P180_CommandSource_e::PluginRead; // To allow logging
    _commands      = parseI2CCommands(EMPTY_STRING, _strings[P180_BUFFER_ENTRY_EXIT]);

    if (!_commands.empty()) {
      if (loglevelActiveFor(LOG_LEVEL_INFO) && _showLog) {
        addLog(LOG_LEVEL_INFO, strformat(F("P180 : Device shutdown/exit with %d commands."), _commands.size()));
      }
      _commandState = P180_CommandState_e::Processing;
      _initialized  = executeI2CCommands();
      _commands.clear();
      _commandState = P180_CommandState_e::Idle;
    }
    _commandSource = P180_CommandSource_e::PluginIdle;
  }
}

bool P180_data_struct::loadStrings(struct EventStruct*event) {
  bool result = false;

  if (!_stringsLoaded) {
    LoadCustomTaskSettings(event->TaskIndex, _strings, P180_CUSTOM_BUFFER_SIZE, 0);
    _stringsLoaded = true;
  }
  return result;
}

/*********************************************************************************************
 * Handle I2C-command processing, optionally stored to command cache
 * Format: <cmd>.<dataformat>[.<len>][.<register>][.<data>];...
 ********************************************************************************************/
std::vector<P180_Command_struct>P180_data_struct::parseI2CCommands(const String& name,
                                                                   const String& line) {
  return parseI2CCommands(name, line, false);
}

std::vector<P180_Command_struct>P180_data_struct::parseI2CCommands(const String& name,
                                                                   const String& line,
                                                                   const bool    update) {
  std::vector<P180_Command_struct> commands;

  const String key = parseString(name, 1);
  String keyPostfix;

  if (!key.isEmpty() && (_commandCache.count(key) == 1) && !update) {
    commands = _commandCache.find(key)->second;

    if (loglevelActiveFor(LOG_LEVEL_INFO) && _showLog && (P180_CommandSource_e::PluginRead == _commandSource)) {
      addLog(LOG_LEVEL_INFO, strformat(F("P180 : Retrieve '%s' from cache with %d commands."), name.c_str(), commands.size()));
    }
  }

  if (!line.isEmpty() && ((commands.empty()) || update) && (P180_CommandSource_e::PluginRead == _commandSource)) {
    int evt = 1;

    while (evt > 0) {
      keyPostfix.clear();
      String evtAll = parseStringKeepCaseNoTrim(line, evt, P180_EVENT_SEPARATOR);

      if (evtAll.equalsIgnoreCase(F("1ps"))) { // PLUGIN_ONCE_A_SECOND
        ++evt;
        keyPostfix = cacheSuffix(P180_CommandSource_e::PluginOncePerSecond);
        _has1ps    = true;
      } else
      if (evtAll.equalsIgnoreCase(F("10ps"))) { // PLUGIN_TEN_PER_SECOND
        ++evt;
        keyPostfix = cacheSuffix(P180_CommandSource_e::PluginTenPerSecond);
        _has10ps   = true;
      } else
      if (evtAll.equalsIgnoreCase(F("50ps"))) { // PLUGIN_FIFTY_PER_SECOND
        ++evt;
        keyPostfix = cacheSuffix(P180_CommandSource_e::PluginFiftyPerSecond);
        _has50ps   = true;
      }

      if (!keyPostfix.isEmpty()) { // Next part is I2C command sequence
        evtAll = parseStringKeepCaseNoTrim(line, evt, P180_EVENT_SEPARATOR);
      }

      if (!evtAll.isEmpty()) {
        ++evt;
        commands.clear();
      } else {
        evt = 0;
      }

      // parse line
      int idx = 1;

      while (idx > 0 && evt > 0) {
        const String cmdAll = parseStringKeepCaseNoTrim(evtAll, idx, P180_COMMAND_SEPARATOR);
        bool addCmd         = true;

        if (!cmdAll.isEmpty()) {
          std::vector<String> args;

          uint8_t i    = 1;
          String  arg0 = parseStringKeepCaseNoTrim(cmdAll, i, P180_ARGUMENT_SEPARATOR);

          while (i < 5 || !arg0.isEmpty()) { // Read at least 4 arguments
            args.push_back(arg0);
            ++i;
            arg0 = parseStringKeepCaseNoTrim(cmdAll, i, P180_ARGUMENT_SEPARATOR);
          }

          # ifndef LIMIT_BUILD_SIZE

          if (loglevelActiveFor(LOG_LEVEL_INFO) && _showLog && (P180_CommandSource_e::PluginRead == _commandSource)) {
            addLog(LOG_LEVEL_INFO, strformat(F("P180 : Arguments parsed: %d (%s)"), args.size(), cmdAll.c_str()));
          }
          # endif // ifndef LIMIT_BUILD_SIZE

          args[0].toLowerCase();
          String sFmt = args[1];
          sFmt.toLowerCase();
          const int arg2i = GetCommandCode(sFmt.c_str(), P180_dataFormats);
          int arg1i       = GetCommandCode(args[0].c_str(), P180_commands);

          if (arg1i < 0) {
            arg1i = GetCommandCode(args[0].c_str(), P180_commandsLong);
          }
          uint8_t arg = 2;

          uint16_t reg          = 0;
          uint32_t len          = 0;
          P180_DataFormat_e fmt = P180_DataFormat_e::uint8_t;
          String calculation;
          String variable;

          if (arg2i > -1) {
            fmt = static_cast<P180_DataFormat_e>(arg2i);
          }

          if ((P180_DataFormat_e::bytes == fmt) || (P180_DataFormat_e::words == fmt)) {
            validUIntFromString(args[arg], len);
            ++arg;
          }

          if (arg1i > -1) {
            P180_Command_e cmd = static_cast<P180_Command_e>(arg1i);
            int64_t val        = 0;
            validInt64FromString(args[arg], val);

            switch (cmd) {
              case P180_Command_e::NOP: break;
              case P180_Command_e::Read:      // get - g.<format>
              case P180_Command_e::Write:     // put - p.<format>.<value>
                break;
              case P180_Command_e::Calculate: // calc - c.<calculation>
              case P180_Command_e::If:        // if - i.<calculation>
                fmt         = P180_DataFormat_e::undefined;
                val         = 0;
                calculation = args[arg - 1];
                stripEscapeCharacters(calculation);
                break;
              case P180_Command_e::Let: // let - l.<variable>.<calculation>
                fmt         = P180_DataFormat_e::undefined;
                val         = 0;
                variable    = args[arg - 1];
                calculation = args[arg];
                stripEscapeCharacters(variable);
                stripEscapeCharacters(calculation);
                break;
              case P180_Command_e::Eval:  // eval - e
                fmt = P180_DataFormat_e::undefined;
                break;
              case P180_Command_e::Value: // value - v.<valueIndex>
              case P180_Command_e::Delay: // delay - d.<ms>
              {
                fmt = P180_DataFormat_e::undefined;
                const bool isInt = validInt64FromString(args[arg - 1], val);

                if ((P180_Command_e::Value == cmd) && !isInt) {
                  val = findDeviceValueIndexByName(args[arg - 1], _taskIndex);
                }
                break;
              }
              case P180_Command_e::RegisterRead:   // read - r.<format>.<reg>
              case P180_Command_e::Register16Read: // read16 - s.<format>.<reg16>
                reg = val;
                val = 0;
                break;
              case P180_Command_e::RegisterWrite:   // write - w.<format>.<reg>.<data>
              case P180_Command_e::Register16Write: // write16 - t.<format>.<reg16>.<data>
                reg = val;
                val = 0;

                if (!((P180_DataFormat_e::bytes == fmt) || (P180_DataFormat_e::words == fmt))) {
                  ++arg;

                  validInt64FromString(args[arg], val);
                }
                break;
              case P180_Command_e::EnableGPIO: // enable - l.<state>
              case P180_Command_e::ResetGPIO:  // reset - z.<state>.<msec>
                fmt = P180_DataFormat_e::undefined;
                val = 0;
                validInt64FromString(args[arg - 1], val);

                if ((val < 0) || (val > 1)) { // Range check
                  addCmd = false;
                } else {
                  reg = val;                  // State

                  if (P180_Command_e::ResetGPIO == cmd) {
                    val = 0;

                    if (!validInt64FromString(args[arg], val) || (val < 0)) { // msec
                      addCmd = false;
                    }
                  }
                }
                break;
            }

            if (addCmd) {
              commands.push_back(P180_Command_struct(cmd, fmt, reg, val, len, calculation, variable));

              if (((P180_Command_e::Write == cmd) || (P180_Command_e::RegisterWrite == cmd) || (P180_Command_e::Register16Write == cmd)) &&
                  ((P180_DataFormat_e::bytes == fmt) || (P180_DataFormat_e::words == fmt))) {
                const size_t currentCommandsIdx = commands.size() - 1;

                while (arg < args.size()) {
                  uint32_t val = 0;

                  if (validUIntFromString(args[arg], val)) {
                    if (P180_DataFormat_e::bytes == fmt) { // bytes
                      commands[currentCommandsIdx].data_b.push_back((uint8_t)val);
                    } else {                               // words
                      commands[currentCommandsIdx].data_w.push_back((uint16_t)val);
                    }
                  }
                  ++arg;
                }
              }
            }
          }
          ++idx;   // next
        } else {
          idx = 0; // done
        }
      }

      if (evt > 0) {
        # ifndef LIMIT_BUILD_SIZE

        if (loglevelActiveFor(LOG_LEVEL_INFO) && _showLog) {
          for (auto it = commands.begin(); it != commands.end(); ++it) {
            addLog(LOG_LEVEL_INFO, strformat(F("P180 : Parsing command: %s, name: %s"), it->toString().c_str(), name.c_str()));
          }
        }
        # endif // ifndef LIMIT_BUILD_SIZE

        if (!key.isEmpty()) {
          _commandCache[concat(key, keyPostfix)] = commands;

          if (loglevelActiveFor(LOG_LEVEL_INFO) && _showLog) {
            addLog(LOG_LEVEL_INFO, strformat(F("P180 : Insert '%s%s' into cache with %d commands."),
                                             name.c_str(), keyPostfix.c_str(), commands.size()));
          }
        }
      }
    }
  }

  return commands;
}

/*********************************************************************************************
 * Execute I2C commands
 ********************************************************************************************/
bool P180_data_struct::executeI2CCommands() {
  bool result = false;

  if (P180_CommandState_e::Processing == _commandState) {
    _it = _commands.begin();
  }

  if ((P180_CommandState_e::Processing == _commandState) &&
      (_taskIndex != INVALID_TASK_INDEX) &&
      (_varIndex != INVALID_TASKVAR_INDEX) &&
      !_valueIsSet) {
    _value = UserVar.getFloat(_taskIndex, _varIndex);
  }

  if (P180_CommandState_e::WaitingForDelay == _commandState) { // Returning from a delay
    _commandState = P180_CommandState_e::Processing;

    if (validGpio(_rstPin) && (P180_Command_e::ResetGPIO == _lastCommand)) {
      DIRECT_pinWrite(_rstPin, _lastReg ? LOW : HIGH); // Revert ResetGPIO state
    }
  }

  while (_it != _commands.end() && P180_CommandState_e::Processing == _commandState) {
    bool is_ok = false;
    uint32_t du32{};
    uint16_t du16{};
    _lastCommand = _it->command; // We only need this for long ResetGPIO pulses
    _lastReg     = _it->reg;

    switch (_it->command) {
      case P180_Command_e::NOP: break;
      case P180_Command_e::Read:

        switch (_it->format) {
          case P180_DataFormat_e::undefined: break;
          case P180_DataFormat_e::uint8_t:
            _it->d0_uint8_t = I2C_read8(_i2cAddress, &is_ok);
            break;
          case P180_DataFormat_e::uint16_t:
            _it->d0_uint16_t = I2C_read16(_i2cAddress, &is_ok);
            break;
          case P180_DataFormat_e::uint24_t:
            _it->d0_uint32_t = I2C_read24(_i2cAddress, &is_ok);
            break;
          case P180_DataFormat_e::uint32_t: // Fall through
          case P180_DataFormat_e::int32_t:
            _it->d0_uint32_t = I2C_read32(_i2cAddress, &is_ok);
            break;
          case P180_DataFormat_e::uint16_t_LE:
            du16             = I2C_read16(_i2cAddress, &is_ok);
            _it->d0_uint16_t = (du16 << 8) | (du16 >> 8);
            break;
          case P180_DataFormat_e::uint24_t_LE:
            du32             = I2C_read24(_i2cAddress, &is_ok);
            _it->d0_uint32_t = ((du32 & 0xFF0000) >> 16) | (du32 & 0xFF00) | ((du32 & 0xFF) << 16);
            break;
          case P180_DataFormat_e::uint32_t_LE:
            du32             = I2C_read32(_i2cAddress, &is_ok);
            _it->d0_uint32_t = ((du32 & 0xFF000000) >> 24) | ((du32 & 0xFF0000) >> 16) | ((du32 & 0xFF00) << 8) | ((du32 & 0xFF) << 24);
            break;
          case P180_DataFormat_e::int8_t:
            _it->d0_int8_t = I2C_read8(_i2cAddress, &is_ok);
            break;
          case P180_DataFormat_e::int16_t:
            _it->d0_int16_t = I2C_read16(_i2cAddress, &is_ok);
            break;
          case P180_DataFormat_e::int16_t_LE:
            du16            = I2C_read16(_i2cAddress, &is_ok);
            _it->d0_int16_t = (du16 << 8) | (du16 >> 8);
            break;
          case P180_DataFormat_e::int24_t:
            _it->d0_int32_t = I2C_read24(_i2cAddress, &is_ok);
            break;
          case P180_DataFormat_e::int24_t_LE:
            du32            = I2C_read24(_i2cAddress, &is_ok);
            _it->d0_int32_t = ((du32 & 0xFF0000) >> 16) | (du32 & 0xFF00) | ((du32 & 0xFF) << 16);
            break;
          case P180_DataFormat_e::int32_t_LE:
            du32            = I2C_read32(_i2cAddress, &is_ok);
            _it->d0_int32_t = ((du32 & 0xFF000000) >> 24) | ((du32 & 0xFF0000) >> 16) | ((du32 & 0xFF00) << 8) | ((du32 & 0xFF) << 24);
            break;
          case P180_DataFormat_e::bytes:
            _it->data_b.reserve(_it->len);

            Wire.requestFrom(_i2cAddress, _it->len);

            for (uint8_t i = 0; i < _it->len; ++i) {
              _it->data_b.push_back(Wire.read());
            }
            break;
          case P180_DataFormat_e::words:
            _it->data_w.reserve(_it->len);

            Wire.requestFrom(_i2cAddress, _it->len * 2);

            for (uint8_t i = 0; i < _it->len; ++i) {
              _it->data_w.push_back((Wire.read() << 8) | Wire.read());
            }
            break;
        }
        result = true;
        break;
      case P180_Command_e::Write:

        switch (_it->format) {
          case P180_DataFormat_e::undefined: break;
          case P180_DataFormat_e::uint8_t:
            I2C_write8(_i2cAddress, _it->d0_uint8_t);
            break;
          case P180_DataFormat_e::uint16_t:
            I2C_write16(_i2cAddress, _it->d0_uint16_t);
            break;
          case P180_DataFormat_e::uint24_t:
            I2C_write24(_i2cAddress, _it->d0_uint32_t);
            break;
          case P180_DataFormat_e::uint32_t:
            I2C_write32(_i2cAddress, _it->d0_uint32_t);
            break;
          case P180_DataFormat_e::int32_t:
            I2C_write32(_i2cAddress, _it->d0_int32_t);
            break;
          case P180_DataFormat_e::uint16_t_LE:
            I2C_write16_LE(_i2cAddress, _it->d0_uint16_t);
            break;
          case P180_DataFormat_e::uint24_t_LE:
            du32 = ((_it->d0_uint32_t & 0xFF0000) >> 16) | (_it->d0_uint32_t & 0xFF00) | ((_it->d0_uint32_t & 0xFF) << 16);
            I2C_write24(_i2cAddress, du32);
            break;
          case P180_DataFormat_e::uint32_t_LE:
            du32 = ((_it->d0_uint32_t & 0xFF000000) >> 24) | ((_it->d0_uint32_t & 0xFF0000) >> 16) |
                   (_it->d0_uint32_t & 0xFF00) | ((_it->d0_uint32_t & 0xFF) << 16);
            I2C_write32(_i2cAddress, du32);
            break;
          case P180_DataFormat_e::int8_t:
            I2C_write8(_i2cAddress, _it->d0_int8_t);
            break;
          case P180_DataFormat_e::int16_t:
            I2C_write16(_i2cAddress, _it->d0_int16_t);
            break;
          case P180_DataFormat_e::int16_t_LE:
            I2C_write16_LE(_i2cAddress, _it->d0_int16_t);
            break;
          case P180_DataFormat_e::int24_t:
            I2C_write24(_i2cAddress, _it->d0_int32_t);
            break;
          case P180_DataFormat_e::int24_t_LE:
            du32 = ((_it->d0_int32_t & 0xFF0000) >> 16) | (_it->d0_int32_t & 0xFF00) | ((_it->d0_int32_t & 0xFF) << 16);
            I2C_write24(_i2cAddress, du32);
            break;
          case P180_DataFormat_e::int32_t_LE:
            du32 = ((_it->d0_int32_t & 0xFF000000) >> 24) | ((_it->d0_int32_t & 0xFF0000) >> 16) |
                   (_it->d0_int32_t & 0xFF00) | ((_it->d0_int32_t & 0xFF) << 16);
            I2C_write32(_i2cAddress, du32);
            break;
          case P180_DataFormat_e::bytes:
            Wire.beginTransmission(_i2cAddress);

            for (size_t itb = 0; itb < _it->data_b.size(); ++itb) {
              Wire.write(_it->data_b[itb]);
            }
            result = Wire.endTransmission() == 0;
            break;
          case P180_DataFormat_e::words:
            Wire.beginTransmission(_i2cAddress);

            for (size_t itw = 0; itw < _it->data_w.size(); ++itw) {
              Wire.write((uint8_t)(_it->data_w[itw] << 8));
              Wire.write((uint8_t)(_it->data_w[itw] & 0xFF));
            }
            result = Wire.endTransmission() == 0;
            break;
        }
        result = true;
        break;
      case P180_Command_e::RegisterRead:

        switch (_it->format) {
          case P180_DataFormat_e::undefined: break;
          case P180_DataFormat_e::uint8_t:
            _it->d0_uint8_t = I2C_read8_reg(_i2cAddress, _it->reg, &is_ok);
            break;
          case P180_DataFormat_e::uint16_t:
            _it->d0_uint16_t = I2C_read16_reg(_i2cAddress, _it->reg, &is_ok);
            break;
          case P180_DataFormat_e::uint24_t:
            _it->d0_uint32_t = I2C_read24_reg(_i2cAddress, _it->reg, &is_ok);
            break;
          case P180_DataFormat_e::uint32_t: // Fall through
          case P180_DataFormat_e::int32_t:
            _it->d0_uint32_t = I2C_read32_reg(_i2cAddress, _it->reg, &is_ok);
            break;
          case P180_DataFormat_e::uint16_t_LE:
            _it->d0_uint16_t = I2C_read16_LE_reg(_i2cAddress, _it->reg, &is_ok);
            break;
          case P180_DataFormat_e::uint24_t_LE:
            du32             = I2C_read24_reg(_i2cAddress, _it->reg, &is_ok);
            _it->d0_uint32_t = ((du32 & 0xFF0000) >> 16) | (du32 & 0xFF00) | ((du32 & 0xFF) << 16);
            break;
          case P180_DataFormat_e::uint32_t_LE:
            du32             = I2C_read32_reg(_i2cAddress, _it->reg, &is_ok);
            _it->d0_uint32_t = ((du32 & 0xFF000000) >> 24) | ((du32 & 0xFF0000) >> 16) | ((du32 & 0xFF00) << 8) | ((du32 & 0xFF) << 24);
            break;
          case P180_DataFormat_e::int8_t:
            _it->d0_int8_t = I2C_read8_reg(_i2cAddress, _it->reg, &is_ok);
            break;
          case P180_DataFormat_e::int16_t:
            _it->d0_int16_t = I2C_read16_reg(_i2cAddress, _it->reg, &is_ok);
            break;
          case P180_DataFormat_e::int16_t_LE:
            du16            = I2C_read16_LE_reg(_i2cAddress, _it->reg, &is_ok);
            _it->d0_int16_t = (du16 << 8) | (du16 >> 8);
            break;
          case P180_DataFormat_e::int24_t:
            _it->d0_int32_t = I2C_read24_reg(_i2cAddress, _it->reg, &is_ok);
            break;
          case P180_DataFormat_e::int24_t_LE:
            du32            = I2C_read24_reg(_i2cAddress, _it->reg, &is_ok);
            _it->d0_int32_t = ((du32 & 0xFF0000) >> 16) | (du32 & 0xFF00) | ((du32 & 0xFF) << 16);
            break;
          case P180_DataFormat_e::int32_t_LE:
            du32            = I2C_read32_reg(_i2cAddress, _it->reg, &is_ok);
            _it->d0_int32_t = ((du32 & 0xFF000000) >> 24) | ((du32 & 0xFF0000) >> 16) | ((du32 & 0xFF00) << 8) | ((du32 & 0xFF) << 24);
            break;
          case P180_DataFormat_e::bytes:
            _it->data_b.reserve(_it->len);

            if (I2C_write8(_i2cAddress, _it->reg) && (Wire.requestFrom(_i2cAddress, _it->len) == _it->len)) {
              for (uint8_t i = 0; i < _it->len; ++i) {
                _it->data_b.push_back(Wire.read());
              }
            }
            break;
          case P180_DataFormat_e::words:
            _it->data_w.reserve(_it->len);

            if (I2C_write8(_i2cAddress, _it->reg) && (Wire.requestFrom(_i2cAddress, _it->len * 2) == _it->len * 2)) {
              for (uint8_t i = 0; i < _it->len; ++i) {
                _it->data_w.push_back((Wire.read() << 8) | Wire.read());
              }
            }
            break;
        }
        result = true;
        break;
      case P180_Command_e::Register16Read:
        addLog(LOG_LEVEL_ERROR, F("P180 : Register16Read (read16/s) feature not implemented yet."));   // FIXME ??
        break;
      case P180_Command_e::Register16Write:
        addLog(LOG_LEVEL_ERROR, F("P180 : Register16Write (write16/t) feature not implemented yet.")); // FIXME ??
        break;
      case P180_Command_e::RegisterWrite:

        switch (_it->format) {
          case P180_DataFormat_e::undefined: break;
          case P180_DataFormat_e::uint8_t:
            I2C_write8_reg(_i2cAddress, _it->reg, _it->d0_uint8_t);
            break;
          case P180_DataFormat_e::uint16_t:
            I2C_write16_reg(_i2cAddress, _it->reg, _it->d0_uint16_t);
            break;
          case P180_DataFormat_e::uint24_t:
            I2C_write24_reg(_i2cAddress, _it->reg, _it->d0_uint32_t);
            break;
          case P180_DataFormat_e::uint32_t: // Fall through
            I2C_write32_reg(_i2cAddress, _it->reg, _it->d0_uint32_t);
            break;
          case P180_DataFormat_e::int32_t:
            I2C_write32_reg(_i2cAddress, _it->reg, _it->d0_int32_t);
            break;
          case P180_DataFormat_e::uint16_t_LE:
            I2C_write16_LE_reg(_i2cAddress, _it->reg, _it->d0_uint16_t);
            break;
          case P180_DataFormat_e::uint24_t_LE:
            I2C_write24_LE_reg(_i2cAddress, _it->reg, _it->d0_uint32_t);
            break;
          case P180_DataFormat_e::uint32_t_LE:
            I2C_write32_LE_reg(_i2cAddress, _it->reg, _it->d0_uint32_t);
            break;
          case P180_DataFormat_e::int8_t:
            I2C_write8_reg(_i2cAddress, _it->reg, _it->d0_int8_t);
            break;
          case P180_DataFormat_e::int16_t:
            I2C_write16_reg(_i2cAddress, _it->reg, _it->d0_int16_t);
            break;
          case P180_DataFormat_e::int16_t_LE:
            I2C_write16_LE_reg(_i2cAddress, _it->reg, _it->d0_int16_t);
            break;
          case P180_DataFormat_e::int24_t:
            I2C_write24_reg(_i2cAddress, _it->reg, _it->d0_int32_t);
            break;
          case P180_DataFormat_e::int24_t_LE:
            I2C_write24_LE_reg(_i2cAddress, _it->reg, _it->d0_int32_t);
            break;
          case P180_DataFormat_e::int32_t_LE:
            I2C_write32_reg(_i2cAddress, _it->reg, _it->d0_int32_t);
            break;
          case P180_DataFormat_e::bytes:
            Wire.beginTransmission(_i2cAddress);
            Wire.write((uint8_t)_it->reg);

            for (size_t itb = 0; itb < _it->data_b.size(); ++itb) {
              Wire.write(_it->data_b[itb]);
            }
            result = Wire.endTransmission() == 0;
            break;
          case P180_DataFormat_e::words:
            Wire.beginTransmission(_i2cAddress);
            Wire.write((uint8_t)_it->reg);

            for (size_t itw = 0; itw < _it->data_w.size(); ++itw) {
              Wire.write((uint8_t)(_it->data_w[itw] << 8));
              Wire.write((uint8_t)(_it->data_w[itw] & 0xFF));
            }
            result = Wire.endTransmission() == 0;
            break;
        }
        result = true;
        break;
      case P180_Command_e::Value:

        if ((_it->d0_uint32_t > 0) && (_it->d0_uint32_t <= VARS_PER_TASK)) {
          UserVar.setFloat(_taskIndex, _it->d0_uint32_t - 1, _value);
          _valueIsSet = true;
        }
        result = true;
        break;
      case P180_Command_e::Calculate:
      case P180_Command_e::If:
      case P180_Command_e::Let:

        result = true;

        if (!_it->calculation.isEmpty()) {
          String toCalc(replacePluginValues(_it->calculation));
          const String newCalc = parseTemplate(toCalc); // Process like rules
          ESPEASY_RULES_FLOAT_TYPE tmp{};

          if (Calculate(newCalc, tmp) == CalculateReturnCode::OK) {
            if (loglevelActiveFor(LOG_LEVEL_INFO) && _showLog && (P180_CommandSource_e::PluginRead == _commandSource)) {
              addLog(LOG_LEVEL_INFO, strformat(F("P180 : Calculation: %s, result: %s"), toCalc.c_str(), doubleToString(tmp).c_str()));
            }

            if (P180_Command_e::If == _it->command) {
              if (essentiallyZero(tmp)) { // 0 = false => cancel execution
                _commandState = P180_CommandState_e::ConditionalExit;
                result        = false;    // PLUGIN_READ failed
              }
            } else if (P180_Command_e::Let == _it->command) {
              String toVar(replacePluginValues(_it->variable));
              const String newVar = parseTemplate(toVar); // Process like rules

              if (!newVar.isEmpty() && ExtraTaskSettings.checkInvalidCharInNames(newVar.c_str())) {
                setCustomFloatVar(newVar, tmp);
              } else {
                result = false;
              }
            } else {
              _value = tmp;
            }
          }
        }
        break;
      case P180_Command_e::Eval:

        if (_it != _commands.begin()) { // No previous entry
          _evalCommand = _it;
          --_evalCommand;               // Previous command holds the desired value
          _evalIsSet = true;
        }
        result = true;
        break;
      case P180_Command_e::Delay:
      {
        const uint32_t mx = ((P180_CommandSource_e::PluginOncePerSecond == _commandSource) ||
                             (P180_CommandSource_e::PluginTenPerSecond == _commandSource) ||
                             (P180_CommandSource_e::PluginFiftyPerSecond == _commandSource)) ? 10u : 500u;
        const uint32_t ms = min(_it->d0_uint32_t, mx); // Reasonable limit, max 10 msec for repeating events

        if (ms <= 10) {
          delay(ms);
          result = true;
        } else {
          _commandState = P180_CommandState_e::StartingDelay;
          Scheduler.schedule_task_device_timer(_taskIndex, millis() + ms);
          result = false; // Don't publish result yet
        }
        break;
      }
      case P180_Command_e::EnableGPIO:

        if (validGpio(_enPin)) {
          DIRECT_pinWrite(_enPin, _it->reg ? HIGH : LOW);
          result = true;
        }
        break;
      case P180_Command_e::ResetGPIO:

        if (validGpio(_rstPin)) {
          const uint32_t ms = min(_it->d0_uint32_t, static_cast<uint32_t>(500u)); // Reasonable limit

          DIRECT_pinWrite(_rstPin, _it->reg ? HIGH : LOW);                        // Set ResetGPIO state

          if (ms <= 10) {
            delay(ms);
            DIRECT_pinWrite(_rstPin, _it->reg ? LOW : HIGH); // Revert ResetGPIO state
            result = true;
          } else {
            _commandState = P180_CommandState_e::StartingDelay;
            Scheduler.schedule_task_device_timer(_taskIndex, millis() + ms);
            result = false; // Don't publish result yet
          }
        }
        break;
    }

    if ((P180_CommandState_e::Processing == _commandState) && !_valueIsSet) {
      switch (_it->format) {
        case P180_DataFormat_e::undefined: break;
        case P180_DataFormat_e::uint8_t:
        case P180_DataFormat_e::uint16_t:
        case P180_DataFormat_e::uint16_t_LE:
        case P180_DataFormat_e::uint24_t:
        case P180_DataFormat_e::uint32_t:
        case P180_DataFormat_e::uint24_t_LE:
        case P180_DataFormat_e::uint32_t_LE:
          _value = _it->getUIntValue();
          break;
        case P180_DataFormat_e::int8_t:
        case P180_DataFormat_e::int16_t:
        case P180_DataFormat_e::int16_t_LE:
        case P180_DataFormat_e::int24_t:
        case P180_DataFormat_e::int32_t:
        case P180_DataFormat_e::int24_t_LE:
        case P180_DataFormat_e::int32_t_LE:
          _value = _it->getIntValue();
          break;
        case P180_DataFormat_e::bytes:
        case P180_DataFormat_e::words:
          break;
      }
    }

    if (loglevelActiveFor(LOG_LEVEL_INFO) && _showLog && (P180_CommandSource_e::PluginRead == _commandSource)) {
      # if FEATURE_USE_DOUBLE_AS_ESPEASY_RULES_FLOAT_TYPE
      String valStr = doubleToString(_value, 2, true);
      # else // if FEATURE_USE_DOUBLE_AS_ESPEASY_RULES_FLOAT_TYPE
      String valStr = toString(_value, 2, true);
      # endif // if FEATURE_USE_DOUBLE_AS_ESPEASY_RULES_FLOAT_TYPE

      addLog(LOG_LEVEL_INFO, strformat(F("P180 : Executing command: %s, value[%d]:(%c): %s"),
                                       _it->toString().c_str(), _varIndex, _valueIsSet ? 't' : 'f', valStr.c_str()));
    }
    ++_it; // Next command
  }

  if ((P180_CommandState_e::Processing == _commandState) &&
      (_taskIndex != INVALID_TASK_INDEX) &&
      (_varIndex != INVALID_TASKVAR_INDEX) &&
      !_valueIsSet &&
      (P180_CommandSource_e::PluginRead == _commandSource)) {
    UserVar.setFloat(_taskIndex, _varIndex, _value);
  }

  return result;
}

String P180_data_struct::replacePluginValues(const String& inVar) {
  String result(inVar);

  result.replace(F("%pvalue%"), toString(_value));                       // %pvalue%

  if (_evalIsSet) {
    result.replace(F("%value%"), toString(_evalCommand->getIntValue())); // %value%
    result.replace(F("%h%"),     _evalCommand->getHexValue(true));       // %h%

    if (P180_DataFormat_e::bytes == _evalCommand->format) {
      if (result.indexOf(F("%b")) > -1) {
        for (uint8_t i = 0; i < _evalCommand->data_b.size(); ++i) {
          result.replace(strformat(F("%%b%d%%"), i),  String(_evalCommand->data_b[i]));                   // %b<n>%
          result.replace(strformat(F("%%bx%d%%"), i), formatToHex_no_prefix(_evalCommand->data_b[i], 2)); // %bx<n>%
        }
      }
    } else if (P180_DataFormat_e::words == _evalCommand->format) {
      if (result.indexOf(F("%w")) > -1) {
        for (uint8_t i = 0; i < _evalCommand->data_w.size(); ++i) {
          result.replace(strformat(F("%%w%d%%"), i),  String(_evalCommand->data_w[i]));                   // %w<n>%
          result.replace(strformat(F("%%wx%d%%"), i), formatToHex_no_prefix(_evalCommand->data_w[i], 4)); // %wx<n>%
        }
      }
    }
  }

  return result;
}

bool P180_data_struct::plugin_once_a_second(struct EventStruct *event) {
  if (((P180_CommandSource_e::PluginIdle != _commandSource) &&
       (P180_CommandSource_e::PluginOncePerSecond != _commandSource)) ||
      !_has1ps) {
    return false;
  }
  _commandSource = P180_CommandSource_e::PluginOncePerSecond;
  return processCommands(event);
}

bool P180_data_struct::plugin_ten_per_second(struct EventStruct *event) {
  if (((P180_CommandSource_e::PluginIdle != _commandSource) &&
       (P180_CommandSource_e::PluginTenPerSecond != _commandSource)) ||
      !_has10ps) {
    return false;
  }
  _commandSource = P180_CommandSource_e::PluginTenPerSecond;
  return processCommands(event);
}

bool P180_data_struct::plugin_fifty_per_second(struct EventStruct *event) {
  if (((P180_CommandSource_e::PluginIdle != _commandSource) &&
       (P180_CommandSource_e::PluginFiftyPerSecond != _commandSource)) ||
      !_has50ps) {
    return false;
  }
  _commandSource = P180_CommandSource_e::PluginFiftyPerSecond;
  return processCommands(event);
}

bool P180_data_struct::plugin_read(struct EventStruct *event) {
  if ((P180_CommandSource_e::PluginIdle != _commandSource) &&
      (P180_CommandSource_e::PluginRead != _commandSource)) {
    return false;
  }
  _commandSource = P180_CommandSource_e::PluginRead;
  return processCommands(event);
}

bool P180_data_struct::processCommands(struct EventStruct *event) {
  bool result = true;

  if (P180_CommandState_e::Idle == _commandState) {
    _commandState = P180_CommandState_e::Processing;
    _loop         = 0;
    _loopMax      = P180_NR_OUTPUT_VALUES;
    _value        = 0.0;
    _valueIsSet   = false; // init

    loadStrings(event);

    const String cacheName = _strings[P180_BUFFER_START_CACHE + _loop].isEmpty()
                                      ? EMPTY_STRING
                                      : concat(_strings[P180_BUFFER_START_CACHE + _loop], cacheSuffix(_commandSource));

    _commands = parseI2CCommands(cacheName, // PluginOnce/Ten/Fifty-PerSecond must come from cache
                                 P180_CommandSource_e::PluginRead ==
                                 _commandSource ? _strings[P180_BUFFER_START_COMMANDS + _loop] : EMPTY_STRING);

    _varIndex = _loop;
  }

  while (P180_CommandState_e::StartingDelay != _commandState && P180_CommandState_e::ConditionalExit != _commandState && _loop < _loopMax) {
    if (!_commands.empty()) {
      result &= executeI2CCommands();
    }

    if (P180_CommandState_e::Processing == _commandState) {
      ++_loop;
      const String cacheName = _strings[P180_BUFFER_START_CACHE + _loop].isEmpty()
                                        ? EMPTY_STRING
                                        : concat(_strings[P180_BUFFER_START_CACHE + _loop], cacheSuffix(_commandSource));

      _commands = parseI2CCommands(cacheName, // PluginOnce/Ten/Fifty-PerSecond must come from cache
                                   P180_CommandSource_e::PluginRead ==
                                   _commandSource ? _strings[P180_BUFFER_START_COMMANDS + _loop] : EMPTY_STRING);
      _varIndex = _loop;
    }
  }

  if ((P180_CommandState_e::Processing == _commandState) || (P180_CommandState_e::ConditionalExit == _commandState)) {
    _commandState = P180_CommandState_e::Idle;
    _commands.clear();
    _valueIsSet    = false; // reset afterward
    _commandSource = P180_CommandSource_e::PluginIdle;
  } else if (P180_CommandState_e::StartingDelay == _commandState) {
    _commandState = P180_CommandState_e::WaitingForDelay;
  }

  return result;
}

/*********************************************************************************************
 * Handle command processing
 ********************************************************************************************/
bool P180_data_struct::plugin_write(struct EventStruct *event,
                                    String            & string) {
  bool success     = false;
  const String cmd = parseString(string, 1);

  if (equals(cmd, F("geni2c"))) {
    const String sub     = parseString(string, 2);
    const String par3    = parseStringKeepCaseNoTrim(string, 3);
    const String varName = parseString(string, 4);
    const bool   hasPar3 = !par3.isEmpty();
    const bool   hasPar4 = !varName.isEmpty();
    const bool   hasPar5 = !parseString(string, 5).isEmpty();
    std::vector<P180_Command_struct> cmds;
    taskVarIndex_t taskVar = INVALID_TASKVAR_INDEX;
    uint32_t val{};

    if (hasPar4) {
      if (validUIntFromString(varName, val) && (val > 0) && (val <= VARS_PER_TASK)) {
        taskVar = val - 1;
      } else {
        taskVar = findDeviceValueIndexByName(varName, event->TaskIndex);
      }
    }

    if (equals(sub, F("cmd")) && hasPar3) { // genI2c,cmd,<I2Ccommands>[,<TaskValueIndex>[,<cache_name>]]
      String cacheName;

      if (hasPar5) {
        cacheName = parseString(string, 5);
      }

      cmds = parseI2CCommands(cacheName, par3, !cacheName.isEmpty());
    } else if (equals(sub, F("exec")) && hasPar3) { // genI2c,exec,<cache_name>[,<TaskValueIndex>]
      cmds = parseI2CCommands(par3, EMPTY_STRING);  // Fetch commands from cache by name
    } else if (equals(sub, F("log")) && hasPar3) {  // genI2c,log,<1|0>
      _showLog       = event->Par3 != 0;
      P180_LOG_DEBUG = _showLog ? 1 : 0;
      success        = true;
    }

    if (!cmds.empty() && (P180_CommandState_e::Idle == _commandState)) { // Execute commands when not busy
      _commands     = cmds;
      _varIndex     = taskVar;
      _loop         = 0;
      _loopMax      = _loop + 1; // Process single entry
      _commandState = P180_CommandState_e::Processing;
      Scheduler.schedule_task_device_timer(_taskIndex, millis() + 5);

      success = true;
    }
  }

  return success;
}

#endif // ifdef USES_P180
