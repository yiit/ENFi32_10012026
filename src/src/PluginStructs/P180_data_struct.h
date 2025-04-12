#ifndef PLUGINSTRUCTS_P180_DATA_STRUCT_H
#define PLUGINSTRUCTS_P180_DATA_STRUCT_H

#include "../../_Plugin_Helper.h"
#ifdef USES_P180

# include <map>

# define P180_MAX_NAME_LENGTH     20   // Max name length
# if FEATURE_EXTENDED_CUSTOM_SETTINGS
#  define P180_MAX_COMMAND_LENGTH  600 // Max I2C command sequence length
# else // if FEATURE_EXTENDED_CUSTOM_SETTINGS
#  define P180_MAX_COMMAND_LENGTH  175 // Max I2C command sequence length
# endif // if FEATURE_EXTENDED_CUSTOM_SETTINGS
# define P180_COMMAND_SEPARATOR   ';'
# define P180_ARGUMENT_SEPARATOR  '.'

// Buffer layout:
// - VARS_PER_TASK entries for cache-name
// - VARS_PER_TASK entries for I2C commands per value
// - 1 entry for 1-time initialization
// - 1 entry for de-initialization on shutdown
constexpr uint8_t P180_CUSTOM_BUFFER_SIZE    = ((VARS_PER_TASK * 2) + 2);
constexpr uint8_t P180_BUFFER_START_CACHE    = 0;
constexpr uint8_t P180_BUFFER_START_COMMANDS = VARS_PER_TASK;
constexpr uint8_t P180_BUFFER_ENTRY_INIT     = (VARS_PER_TASK * 2);
constexpr uint8_t P180_BUFFER_ENTRY_EXIT     = ((VARS_PER_TASK * 2) + 1);

# define P180_ENABLE_PIN          CONFIG_PIN1
# define P180_RST_PIN             CONFIG_PIN2

# define P180_I2C_ADDRESS         PCONFIG(0)

# define P180_SENSOR_TYPE_INDEX   1 // PCONFIG(1)
# define P180_NR_OUTPUT_VALUES    getValueCountFromSensorType(static_cast<Sensor_VType>(PCONFIG(P180_SENSOR_TYPE_INDEX)))
# define P180_VALUE_OFFSET        2 // PCONFIG(2) .. PCONFIG(5)
# define P180_LOG_DEBUG           PCONFIG(6)

// The default set of single-value VType options
constexpr uint8_t P180_START_VTYPE = 0;

enum class P180_Command_e : uint8_t {
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
  EnableGPIO,      // 'l'
  ResetGPIO,       // 'z'
};

enum class P180_DataFormat_e : uint8_t {
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

enum class P180_CommandState_e :uint8_t {
  Idle = 0u,
  Processing,
  StartingDelay,
  WaitingForDelay,
};

struct P180_Command_struct {
  ~P180_Command_struct();

  P180_Command_struct(P180_Command_e    _command,
                      P180_DataFormat_e _format,
                      uint16_t          _reg,
                      int64_t           _data,
                      uint32_t          _len,
                      String            _calculation);
  # ifndef LIMIT_BUILD_SIZE
  String   toString();
  # endif // ifndef LIMIT_BUILD_SIZE
  String   getHexValue();
  String   getHexValue(const bool withPrefix);
  int64_t  getIntValue();
  uint32_t getUIntValue() {
    return static_cast<uint32_t>(getIntValue());
  }

  P180_Command_e    command = P180_Command_e::NOP;
  P180_DataFormat_e format  = P180_DataFormat_e::undefined;
  uint16_t          reg{};
  uint32_t          len{};
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
};

struct P180_data_struct : public PluginTaskData_base {
  P180_data_struct(struct EventStruct *event);
  P180_data_struct() = delete;
  virtual ~P180_data_struct();

  bool plugin_init(struct EventStruct *event);
  void plugin_exit(struct EventStruct *event);
  bool plugin_read(struct EventStruct *event);
  bool plugin_write(struct EventStruct *event,
                    String            & string);
  bool plugin_get_config_value(struct EventStruct *event,
                               String            & string);

private:

  bool                            loadStrings(struct EventStruct *event);

  std::vector<P180_Command_struct>parseI2CCommands(const String& name,
                                                   const String& line);
  std::vector<P180_Command_struct>parseI2CCommands(const String& name,
                                                   const String& line,
                                                   const bool    update);
  bool                            executeI2CCommands();

  ESPEASY_RULES_FLOAT_TYPE _value{};
  int16_t                  _enPin;
  int16_t                  _rstPin;
  uint8_t                  _i2cAddress;
  taskIndex_t              _taskIndex     = INVALID_TASK_INDEX;
  taskVarIndex_t           _varIndex      = INVALID_TASKVAR_INDEX;
  bool                     _initialized   = false;
  bool                     _stringsLoaded = false;
  bool                     _valueIsSet    = false;
  bool                     _evalIsSet     = false;
  bool                     _showLog       = false;

  P180_CommandState_e commandState = P180_CommandState_e::Idle;
  P180_Command_e      _lastCommand = P180_Command_e::NOP;
  uint16_t            _lastReg{};
  uint8_t             _loop    = 0;
  uint8_t             _loopMax = VARS_PER_TASK;
  String              _strings[P180_CUSTOM_BUFFER_SIZE];

  std::map<String, std::vector<P180_Command_struct> >_commandCache;
  std::vector<P180_Command_struct>                   _commands;
  std::vector<P180_Command_struct>::iterator         _evalCommand;
  std::vector<P180_Command_struct>::iterator         _it;
};

#endif // ifdef USES_P180
#endif // ifndef PLUGINSTRUCTS_P180_DATA_STRUCT_H
