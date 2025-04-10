#include "../PluginStructs/P180_data_struct.h"

#ifdef USES_P180
# include <GPIO_Direct_Access.h>
# include "../Globals/RulesCalculate.h"

// These commands (not case-sensitive) must have the same order as the P180_Commands_e enum class
const char P180_commands[] PROGMEM =
  "n|g|p|r|w|s|t|e|c|v|d|l|z|";
const char P180_commandsLong[] PROGMEM =
  "nop|get|put|read|write|read16|write16|eval|calc|value|delay|enable|reset|";

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
                                         String            _calculation)
  :command(_command), format(_format), reg(_reg), len(_len), calculation(_calculation) {
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
  char cmdS[10]{};
  char fmt[10]{};

  GetTextIndexed(cmd,  sizeof(cmd),  static_cast<uint32_t>(command), P180_commandsLong);
  GetTextIndexed(cmdS, sizeof(cmdS), static_cast<uint32_t>(command), P180_commands);
  GetTextIndexed(fmt,  sizeof(fmt),  static_cast<uint32_t>(format),  P180_dataFormats);
  String data;

  if ((P180_DataFormat_e::bytes == format) || (P180_DataFormat_e::words == format)) {
    data = getHexValue(true);
  }

  String result = strformat(F("cmd: '%s' (%s), fmt: '%s', reg: 0x%02x, data: %d (0x%x), len: %d"),
                            cmd, cmdS, fmt, reg, getIntValue(), getUIntValue(), len);

  if (!data.isEmpty()) {
    result = concat(result, strformat(F(", data_b/w: %s"), data.c_str()));
  }

  if (!calculation.isEmpty()) {
    result = concat(result, strformat(F(", calculation: %s"), calculation.c_str()));
  }
  return result;
}

# endif // ifndef LIMIT_BUILD_SIZE

P180_data_struct::P180_data_struct(struct EventStruct *event) {
  _taskIndex  = event->TaskIndex;
  _i2cAddress = P180_I2C_ADDRESS;
  _enPin      = P180_ENABLE_PIN;
  _rstPin     = P180_RST_PIN;
  _showLog    = P180_LOG_DEBUG == 1;
}

P180_data_struct::~P180_data_struct() {
  //
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
    _commands = parseI2CCommands(EMPTY_STRING, _strings[P180_BUFFER_ENTRY_INIT]);

    if (!_commands.empty()) {
      if (loglevelActiveFor(LOG_LEVEL_INFO) && _showLog) {
        addLog(LOG_LEVEL_INFO, strformat(F("P180 : Device initialization with %d commands."), _commands.size()));
      }
      commandState = P180_CommandState_e::Processing;
      _initialized = executeI2CCommands();
      _commands.clear();
      commandState = P180_CommandState_e::Idle;
    }
  }

  return _initialized;
}

void P180_data_struct::plugin_exit(struct EventStruct *event) {
  loadStrings(event);

  if (_stringsLoaded && !_strings[P180_BUFFER_ENTRY_EXIT].isEmpty()) {
    _commands = parseI2CCommands(EMPTY_STRING, _strings[P180_BUFFER_ENTRY_EXIT]);

    if (!_commands.empty()) {
      if (loglevelActiveFor(LOG_LEVEL_INFO) && _showLog) {
        addLog(LOG_LEVEL_INFO, strformat(F("P180 : Device shutdown/exit with %d commands."), _commands.size()));
      }
      commandState = P180_CommandState_e::Processing;
      _initialized = executeI2CCommands();
      _commands.clear();
      commandState = P180_CommandState_e::Idle;
    }
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

  if (!key.isEmpty() && _commandCache.contains(key) && !update) {
    commands = _commandCache.find(key)->second;

    if (loglevelActiveFor(LOG_LEVEL_INFO) && _showLog) {
      addLog(LOG_LEVEL_INFO, strformat(F("P180 : Retrieve '%s' from cache with %d commands."), name.c_str(), commands.size()));
    }
  }

  if (!line.isEmpty() && ((commands.empty()) || update)) {
    commands.clear();

    // parse line
    int idx = 1;

    while (idx > 0) {
      const String cmdAll = parseStringKeepCaseNoTrim(line, idx, P180_COMMAND_SEPARATOR);
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

        if (loglevelActiveFor(LOG_LEVEL_INFO) && _showLog) {
          addLog(LOG_LEVEL_INFO, strformat(F("P180 : Arguments parsed: %d (%s)"), args.size(), cmdAll.c_str()));
        }

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
              fmt         = P180_DataFormat_e::undefined;
              val         = 0;
              calculation = args[arg - 1];
              stripEscapeCharacters(calculation);
              break;
            case P180_Command_e::Eval:  // eval - e
              fmt = P180_DataFormat_e::undefined;
              break;
            case P180_Command_e::Value: // value - v.<valueIndex>
            case P180_Command_e::Delay: // delay - d.<ms>
              fmt = P180_DataFormat_e::undefined;
              validInt64FromString(args[arg - 1], val);
              break;
            case P180_Command_e::RegisterRead:   // read - r.<format>.<reg>
            case P180_Command_e::Register16Read: // read16 - s.<format>.<reg16>
              reg = val;
              val = 0;
              break;
            case P180_Command_e::RegisterWrite:   // write - w.<format>.<reg>.<data>
            case P180_Command_e::Register16Write: // write16 - t.<format>.<reg16>.<data>
            {
              reg = val;
              val = 0;

              if (!((P180_DataFormat_e::bytes == fmt) || (P180_DataFormat_e::words == fmt))) {
                ++arg;

                validInt64FromString(args[arg], val);
              }
              break;
            }
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
            commands.push_back(P180_Command_struct(cmd, fmt, reg, val, len, calculation));

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

    if (!key.isEmpty()) {
      _commandCache.insert_or_assign(key, commands);

      if (loglevelActiveFor(LOG_LEVEL_INFO) && _showLog) {
        addLog(LOG_LEVEL_INFO, strformat(F("P180 : Insert '%s' into cache with %d commands."), name.c_str(), commands.size()));
      }
    }

    # ifndef LIMIT_BUILD_SIZE

    if (loglevelActiveFor(LOG_LEVEL_INFO) && _showLog) {
      for (auto it = commands.begin(); it != commands.end(); ++it) {
        addLog(LOG_LEVEL_INFO, strformat(F("P180 : Parsing command: %s, name: %s"), it->toString().c_str(), name.c_str()));
      }
    }
    # endif // ifndef LIMIT_BUILD_SIZE
  }

  return commands;
}

/*********************************************************************************************
 * Execute I2C commands
 ********************************************************************************************/
bool P180_data_struct::executeI2CCommands() {
  bool result = false;

  if (P180_CommandState_e::Processing == commandState) {
    _it = _commands.begin();
  }

  if ((P180_CommandState_e::Processing == commandState) &&
      (_taskIndex != INVALID_TASK_INDEX) &&
      (_varIndex != INVALID_TASKVAR_INDEX) &&
      !_valueIsSet) {
    _value      = UserVar.getFloat(_taskIndex, _varIndex);
    _valueIsSet = false;                                      // init
  }

  if (P180_CommandState_e::WaitingForDelay == commandState) { // Returning from a delay
    commandState = P180_CommandState_e::Processing;

    if (validGpio(_rstPin) && (P180_Command_e::ResetGPIO == _lastCommand)) {
      DIRECT_pinWrite(_rstPin, _lastReg ? LOW : HIGH); // Revert ResetGPIO state
    }
  }

  while (_it != _commands.end() && P180_CommandState_e::Processing == commandState) {
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
          {
            _it->data_b.reserve(_it->len);

            Wire.requestFrom(_i2cAddress, _it->len);

            for (uint8_t i = 0; i < _it->len; ++i) {
              _it->data_b.push_back(Wire.read());
            }
            break;
          }
          case P180_DataFormat_e::words:
          {
            _it->data_w.reserve(_it->len);

            Wire.requestFrom(_i2cAddress, _it->len * 2);

            for (uint8_t i = 0; i < _it->len; ++i) {
              _it->data_w.push_back((Wire.read() << 8) | Wire.read());
            }
            break;
          }
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
          {
            Wire.beginTransmission(_i2cAddress);

            for (size_t itb = 0; itb < _it->data_b.size(); ++itb) {
              Wire.write(_it->data_b[itb]);
            }
            result = Wire.endTransmission() == 0;
            break;
          }
          case P180_DataFormat_e::words:
          {
            Wire.beginTransmission(_i2cAddress);

            for (size_t itw = 0; itw < _it->data_w.size(); ++itw) {
              Wire.write((uint8_t)(_it->data_w[itw] << 8));
              Wire.write((uint8_t)(_it->data_w[itw] & 0xFF));
            }
            result = Wire.endTransmission() == 0;
            break;
          }
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
          {
            _it->data_b.reserve(_it->len);

            if (I2C_write8(_i2cAddress, _it->reg) && (Wire.requestFrom(_i2cAddress, _it->len) == _it->len)) {
              for (uint8_t i = 0; i < _it->len; ++i) {
                _it->data_b.push_back(Wire.read());
              }
            }
            break;
          }
          case P180_DataFormat_e::words:
          {
            _it->data_w.reserve(_it->len);

            if (I2C_write8(_i2cAddress, _it->reg) && (Wire.requestFrom(_i2cAddress, _it->len * 2) == _it->len * 2)) {
              for (uint8_t i = 0; i < _it->len; ++i) {
                _it->data_w.push_back((Wire.read() << 8) | Wire.read());
              }
            }
            break;
          }
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
          {
            Wire.beginTransmission(_i2cAddress);
            Wire.write((uint8_t)_it->reg);

            for (size_t itb = 0; itb < _it->data_b.size(); ++itb) {
              Wire.write(_it->data_b[itb]);
            }
            result = Wire.endTransmission() == 0;
            break;
          }
          case P180_DataFormat_e::words:
          {
            Wire.beginTransmission(_i2cAddress);
            Wire.write((uint8_t)_it->reg);

            for (size_t itw = 0; itw < _it->data_w.size(); ++itw) {
              Wire.write((uint8_t)(_it->data_w[itw] << 8));
              Wire.write((uint8_t)(_it->data_w[itw] & 0xFF));
            }
            result = Wire.endTransmission() == 0;
            break;
          }
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

        if (!_it->calculation.isEmpty()) {
          String toCalc(_it->calculation);

          toCalc.replace(F("%pvalue%"), toString(_value));                                         // %pvalue%

          if (_evalIsSet) {
            toCalc.replace(F("%value%"), toString(_evalCommand->getIntValue()));                   // %value%

            if (P180_DataFormat_e::bytes == _evalCommand->format) {
              toCalc.replace(F("%h%"), _evalCommand->getHexValue(true));                           // %h%

              for (uint8_t i = 0; i < _evalCommand->data_b.size(); ++i) {
                toCalc.replace(strformat(F("%%b%d%%"), i),  String(_evalCommand->data_b[i]));      // %b<n>%
                toCalc.replace(strformat(F("%%bx%d%%"), i), formatToHex(_evalCommand->data_b[i])); // %bx<n>%
              }
            } else if (P180_DataFormat_e::words == _evalCommand->format) {
              toCalc.replace(F("%%h%%"), _evalCommand->getHexValue());                             // %h%

              for (uint8_t i = 0; i < _evalCommand->data_w.size(); ++i) {
                toCalc.replace(strformat(F("%%w%d%%"), i),  String(_evalCommand->data_w[i]));      // %w<n>%
                toCalc.replace(strformat(F("%%wx%d%%"), i), formatToHex(_evalCommand->data_w[i])); // %wx<n>%
              }
            }
          }
          String newCalc = parseTemplate(toCalc); // Process like rules
          ESPEASY_RULES_FLOAT_TYPE tmp{};

          if (Calculate(newCalc, tmp) == CalculateReturnCode::OK) {
            _value = tmp;
          }
        }
        result = true;
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
        const uint32_t ms = min(_it->d0_uint32_t, static_cast<uint32_t>(500u)); // Reasonable limit

        if (ms <= 10) {
          delay(ms);
          result = true;
        } else {
          commandState = P180_CommandState_e::StartingDelay;
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
      {
        if (validGpio(_rstPin)) {
          const uint32_t ms = min(_it->d0_uint32_t, static_cast<uint32_t>(500u)); // Reasonable limit

          DIRECT_pinWrite(_rstPin, _it->reg ? HIGH : LOW);                        // Set ResetGPIO state

          if (ms <= 10) {
            delay(ms);
            DIRECT_pinWrite(_rstPin, _it->reg ? LOW : HIGH); // Revert ResetGPIO state
            result = true;
          } else {
            commandState = P180_CommandState_e::StartingDelay;
            Scheduler.schedule_task_device_timer(_taskIndex, millis() + ms);
            result = false; // Don't publish result yet
          }
        }
        break;
      }
    }

    if ((P180_CommandState_e::Processing == commandState) && !_valueIsSet) {
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
    # if FEATURE_USE_DOUBLE_AS_ESPEASY_RULES_FLOAT_TYPE
    String valStr = doubleToString(_value, 2, true);
    # else // if FEATURE_USE_DOUBLE_AS_ESPEASY_RULES_FLOAT_TYPE
    String valStr = toString(_value, 2, true);
    # endif // if FEATURE_USE_DOUBLE_AS_ESPEASY_RULES_FLOAT_TYPE

    if (loglevelActiveFor(LOG_LEVEL_INFO) && _showLog) {
      addLog(LOG_LEVEL_INFO, strformat(F("P180 : Executing command: %s, value: %s"),
                                       _it->toString().c_str(), valStr.c_str()));
    }
    ++_it; // Next command
  }

  if ((P180_CommandState_e::Processing == commandState) &&
      (_taskIndex != INVALID_TASK_INDEX) &&
      (_varIndex != INVALID_TASKVAR_INDEX) &&
      !_valueIsSet) {
    UserVar.setFloat(_taskIndex, _varIndex, _value);
  }

  return result;
}

bool P180_data_struct::plugin_read(struct EventStruct *event) {
  bool result = true;

  if (P180_CommandState_e::Idle == commandState) {
    commandState = P180_CommandState_e::Processing;
    _loop        = 0;
    _loopMax     = P180_NR_OUTPUT_VALUES;
    _value       = 0.0;

    loadStrings(event);

    _commands = parseI2CCommands(_strings[P180_BUFFER_START_CACHE + _loop], _strings[P180_BUFFER_START_COMMANDS + _loop]);
    _varIndex = _loop + 1;
  }

  while (P180_CommandState_e::StartingDelay != commandState && _loop < _loopMax) {
    if (!_commands.empty()) {
      result &= executeI2CCommands();
    }

    if (P180_CommandState_e::Processing == commandState) {
      ++_loop;
      _commands = parseI2CCommands(_strings[P180_BUFFER_START_CACHE + _loop], _strings[P180_BUFFER_START_COMMANDS + _loop]);
      _varIndex = _loop + 1;
    }
  }

  if (P180_CommandState_e::Processing == commandState) {
    commandState = P180_CommandState_e::Idle;
    _commands.clear();
  } else if (P180_CommandState_e::StartingDelay == commandState) {
    commandState = P180_CommandState_e::WaitingForDelay;
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
    const bool   hasPar3 = !parseString(string, 3).isEmpty();
    const bool   hasPar4 = !parseString(string, 4).isEmpty();
    const bool   hasPar5 = !parseString(string, 5).isEmpty();

    if (equals(sub, F("cmd")) && hasPar3) { // genI2c,cmd,<I2Ccommands>[,<TaskValueIndex>[,<name>]]
      taskVarIndex_t taskVar = INVALID_TASKVAR_INDEX;
      String   name;
      uint32_t val{};

      if (hasPar4 && validUIntFromString(parseString(string, 4), val) && (val > 0) && (val <= VARS_PER_TASK)) {
        taskVar = val - 1;
      }

      if (hasPar5) {
        name = parseString(string, 5);
      }

      std::vector<P180_Command_struct> cmds = parseI2CCommands(name, parseString(string, 3), !name.isEmpty());

      if (!cmds.empty()) {
        _commands    = cmds;
        _varIndex    = taskVar;
        _loop        = 0;
        _loopMax     = _loop + 1; // Process single entry
        commandState = P180_CommandState_e::Processing;
        Scheduler.schedule_task_device_timer(_taskIndex, millis() + 5);

        success = true;
      }
    }
  }

  return success;
}

/*********************************************************************************************
 * Handle get config value processing
 ********************************************************************************************/
bool P180_data_struct::plugin_get_config_value(struct EventStruct *event,
                                               String            & string) {
  bool success = false;

  // const String val = parseString(string, 1);

  // if (equals(val, F("?"))) {
  //   const String sub    = parseString(string, 2);
  //   const bool   hasPar = !parseString(string, 3).isEmpty();
  // }

  return success;
}

#endif // ifdef USES_P180
