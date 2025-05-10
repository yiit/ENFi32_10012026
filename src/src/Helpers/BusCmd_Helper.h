#ifndef _HELPERS_BUSCMD_HELPER_H
#define _HELPERS_BUSCMD_HELPER_H

#include "../../_Plugin_Helper.h"

#define BUSCMD_EVENT_SEPARATOR     '|'
#define BUSCMD_COMMAND_SEPARATOR   ';'
#define BUSCMD_ARGUMENT_SEPARATOR  '.'

enum class BusCmd_Command_e : uint8_t {
  NOP = 0u,        // 'n'
  Read,            // 'g'
  Write,           // 'p'
  RegisterRead,    // 'r' 8 bit register value
  RegisterWrite,   // 'w'
  Register16Read,  // 's' 16 bit register value
  Register16Write, // 't'
  Eval,            // 'e'
  Calculate,       // 'c'
  Value,           // 'v'
  Delay,           // 'd'
  EnableGPIO,      // 'a'
  ResetGPIO,       // 'z'
  If,              // 'i'
  Let,             // 'l'
};

enum class BusCmd_DataFormat_e : uint8_t {
  undefined = 0u,
  uint8_t,
  uint16_t,
  uint24_t,
  uint32_t,
  uint16_t_LE,
  uint24_t_LE,
  uint32_t_LE,
  int8_t,
  int16_t,
  int24_t,
  int32_t,
  int16_t_LE,
  int24_t_LE,
  int32_t_LE,
  bytes,
  words,
};

enum class BusCmd_CommandState_e :uint8_t {
  Idle = 0u,
  Processing,
  StartingDelay,   // Interrupts the command execution loop
  WaitingForDelay,
  ConditionalExit, // Cancels further command execution
};

enum class BusCmd_CommandSource_e : uint8_t {
  PluginIdle = 0u,
  PluginRead,
  PluginOncePerSecond,
  PluginTenPerSecond,
  PluginFiftyPerSecond,
};

struct BusCmd_Command_struct {
  ~BusCmd_Command_struct();

  BusCmd_Command_struct(BusCmd_Command_e    _command,
                        BusCmd_DataFormat_e _format,
                        uint16_t            _reg,
                        int64_t             _data,
                        uint32_t            _len,
                        String              _calculation,
                        String              _variable);
  #ifndef LIMIT_BUILD_SIZE
  String   toString();
  #endif // ifndef LIMIT_BUILD_SIZE
  String   getHexValue();
  String   getHexValue(const bool withPrefix);
  int64_t  getIntValue();
  uint32_t getUIntValue() {
    return static_cast<uint32_t>(getIntValue());
  }

  BusCmd_Command_e    command = BusCmd_Command_e::NOP;
  BusCmd_DataFormat_e format  = BusCmd_DataFormat_e::undefined;
  uint16_t            reg{};
  uint32_t            len{};
  union {
    struct {
      uint8_t d0_uint8_t;
      uint8_t d1_uint8_t;
      uint8_t d2_uint8_t;
      uint8_t d3_uint8_t;
    };
    struct {
      int8_t d0_int8_t;
      int8_t d1_int8_t;
      int8_t d2_int8_t;
      int8_t d3_int8_t;
    };
    struct {
      uint16_t d0_uint16_t;
      uint16_t d1_uint16_t;
    };
    struct {
      int16_t d0_int16_t;
      int16_t d1_int16_t;
    };
    int32_t  d0_int32_t;
    uint32_t d0_uint32_t{};
  };
  std::vector<uint8_t> data_b;
  std::vector<uint16_t>data_w;
  String               calculation;
  String               variable;
};

struct BusCmd_Buffer {
  BusCmd_Buffer(const String& name,
                const String& line);
  String cacheName;
  String commandSet;
};

struct BusCmd_Helper_struct {
  BusCmd_Helper_struct();

  BusCmd_Helper_struct(taskIndex_t taskIndex,
                       int16_t     enPin,
                       int16_t     rstPin,
                       uint8_t     loopLimit);
  virtual ~BusCmd_Helper_struct();

  bool                              plugin_read(struct EventStruct *event);
  bool                              plugin_once_a_second(struct EventStruct *event);
  bool                              plugin_ten_per_second(struct EventStruct *event);
  bool                              plugin_fifty_per_second(struct EventStruct *event);


  std::vector<BusCmd_Command_struct>parseBusCmdCommands(const String& name,
                                                        const String& line);
  std::vector<BusCmd_Command_struct>parseBusCmdCommands(const String& name,
                                                        const String& line,
                                                        const bool    update);

  bool executeBusCmdCommands();

  bool parseAndExecute(BusCmd_CommandSource_e source,
                       const String         & line,
                       const String         & logFormat);

  // Setters
  void setLog(bool showLog) {
    _showLog = showLog;
  }

  void setI2CAddress(uint8_t i2cAddress) {
    _i2cAddress = i2cAddress;
  }

  void setCommands(std::vector<BusCmd_Command_struct>commands,
                   taskVarIndex_t                    taskVarIndex,
                   uint8_t                           loopStart,
                   uint8_t                           loopMax,
                   BusCmd_CommandState_e             commandState);

  void setBuffer(uint8_t       index,
                 const String& name,
                 const String& line);

  // Getters
  BusCmd_CommandState_e getCommandState() const {
    return _commandState;
  }

  static const __FlashStringHelper* cacheSuffix(BusCmd_CommandSource_e source);

private:

  String replacePluginValues(const String& inVar);
  bool   processCommands(struct EventStruct *event);


  ESPEASY_RULES_FLOAT_TYPE _value{};

  taskVarIndex_t _varIndex = INVALID_TASKVAR_INDEX;
  taskIndex_t    _taskIndex;
  int16_t        _enPin;
  int16_t        _rstPin;
  uint8_t        _i2cAddress;
  bool           _valueIsSet = false;
  bool           _evalIsSet  = false;
  bool           _showLog    = false;
  bool           _has50ps    = false;
  bool           _has10ps    = false;
  bool           _has1ps     = false;

  BusCmd_CommandState_e  _commandState  = BusCmd_CommandState_e::Idle;
  BusCmd_Command_e       _lastCommand   = BusCmd_Command_e::NOP;
  BusCmd_CommandSource_e _commandSource = BusCmd_CommandSource_e::PluginIdle;
  uint16_t               _lastReg{};
  uint8_t                _loop{};
  uint8_t                _loopMax = VARS_PER_TASK;
  uint8_t                _loopLimit;

  std::map<String, std::vector<BusCmd_Command_struct> >_commandCache;
  std::vector<BusCmd_Command_struct>                   _commands;
  std::vector<BusCmd_Command_struct>::iterator         _evalCommand;
  std::vector<BusCmd_Command_struct>::iterator         _it;
  std::vector<BusCmd_Buffer>                           _buffer;
};
#endif // ifndef _HELPERS_BUSCMD_HELPER_H
