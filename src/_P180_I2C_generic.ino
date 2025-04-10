#include "_Plugin_Helper.h"
#ifdef USES_P180

// #######################################################################################################
// ################################## Plugin-180: Generic I2C interface ##################################
// #######################################################################################################

/** Changelog:
 * 2025-03-27 tonhuisman: Start plugin development
 */

/** I2C Commands supported:
 * short : long = Description
 * n : nop = No Operation
 * g : get = Read
 * p : put = write
 * r : read = Read from a register
 * w : write = Write to a register
 * s : read16 = Read from a 16 bit register (not supported yet)
 * t : write16 = Write to a 16 bit register (not supported yet)
 * e : eval = Set previous command data to eval for calc command
 * c : calc = Calculate a result from eval data retrieved
 * v : value = Put current result in Value
 * d : delay = Delay msec. until next command is executed (asynchronous/non-blocking when > 10 msec)
 * q : checksum = Calculate & validate checksum (not supported yet)
 * l : enable = Set Enable GPIO pin to a state
 * z : reset = Pulse Reset GPIO pin to a state for n msec.
 *
 * Command structure: (period . for argument separator, semicolon ; for command separator)
 * <cmd>[.<fmt>[.<len>]][.<register>][.<data>];...
 *
 * get.<fmt>[.<len>]
 * put.<fmt>.<data>[.<data>...]
 * read.<fmt>[.<len>].<reg>
 * write.<fmt>.<reg>.<data>[.<data>...]
 * eval
 * value.<valueIndex> (1..4)
 * calc.<calculation> Like Rules, extra available vars: %value%, %pvalue%, %h%, %b0%..%b<n>%, %w0%..%w<n>%
 * delay.<msec> (range: 0..500)
 *
 * fmt options:
 * u8 : uint8_t (1 byte)
 * u16 : uint16_t (2 bytes)
 * u24 : uint24_t (3 bytes)
 * u32 : uint32_t (4 bytes)
 * u16le : uint16_t (2 bytes, little endian)
 * u24le : uint24_t (3 bytes, little endian)
 * u32le : uint32_t (4 bytes, little endian)
 * 8 : int8_t (1 bytes, signed)
 * 16 : int16_t (2 bytes, signed)
 * 24 : int24_t (3 bytes, signed)
 * 32 : int32_t (4 bytes, signed)
 * 16le : int16_t (2 bytes, signed, little endian)
 * 24le : int24_t (3 bytes, signed, little endian)
 * 32le : int32_t (4 bytes, signed, little endian)
 * b[.<len>] : <len> uint8_t (bytes), <len> is needed for reading only
 * w[.<len>] : <len> uint16_t (words), <len> is needed for reading only
 */

/** Write commands supported:
 * geni2c,cmd,<I2C-commands>[,<TaskVarIndex>][,<cache-name>]
 */

# define PLUGIN_180
# define PLUGIN_ID_180         180
# define PLUGIN_NAME_180       "Generic - I2C Generic"
# define PLUGIN_VALUENAME1_180 "Value1"
# define PLUGIN_VALUENAME2_180 "Value2"
# define PLUGIN_VALUENAME3_180 "Value3"
# define PLUGIN_VALUENAME4_180 "Value4"

# include "src/PluginStructs/P180_data_struct.h"

boolean Plugin_180(uint8_t function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
    {
      auto& dev = Device[++deviceCount];
      dev.Number         = PLUGIN_ID_180;
      dev.Type           = DEVICE_TYPE_I2C;
      dev.VType          = Sensor_VType::SENSOR_TYPE_QUAD;
      dev.ValueCount     = 4;
      dev.SendDataOption = true;
      dev.TimerOption    = true;
      dev.TimerOptional  = true;
      dev.FormulaOption  = true;
      dev.PluginStats    = true;
      break;
    }

    case PLUGIN_GET_DEVICENAME:
    {
      string = F(PLUGIN_NAME_180);
      break;
    }

    case PLUGIN_GET_DEVICEVALUENAMES:
    {
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_180));
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_180));
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_180));
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[3], PSTR(PLUGIN_VALUENAME4_180));
      break;
    }

    case PLUGIN_SET_DEFAULTS:
    {
      // Just some 'random' defaults
      P180_I2C_ADDRESS                = 0x55;
      PCONFIG(P180_SENSOR_TYPE_INDEX) = static_cast<int16_t>(Sensor_VType::SENSOR_TYPE_DUAL);
      break;
    }

    case PLUGIN_GET_DEVICEVALUECOUNT:
    {
      event->Par1 = P180_NR_OUTPUT_VALUES;
      success     = true;
      break;
    }

    case PLUGIN_GET_DEVICEVTYPE:
    {
      event->sensorType = static_cast<Sensor_VType>(PCONFIG(P180_SENSOR_TYPE_INDEX));
      event->idx        = P180_SENSOR_TYPE_INDEX;
      success           = true;
      break;
    }

    case PLUGIN_I2C_HAS_ADDRESS:
    {
      success = (event->Par1 == P180_I2C_ADDRESS);
      break;
    }

    # if FEATURE_I2C_GET_ADDRESS
    case PLUGIN_I2C_GET_ADDRESS:
    {
      event->Par1 = P180_I2C_ADDRESS;
      success     = true;
      break;
    }
    # endif // if FEATURE_I2C_GET_ADDRESS

    case PLUGIN_WEBFORM_SHOW_I2C_PARAMS:
    {
      addFormTextBox(F("I2C Address (Hex)"), F("i2c_addr"),
                     formatToHex_decimal(P180_I2C_ADDRESS), 4);

      break;
    }

    case PLUGIN_WEBFORM_LOAD:
    {
      addFormPinSelect(PinSelectPurpose::Generic_output, F("Enable GPIO"), F("taskdevicepin1"), P180_ENABLE_PIN);
      addFormPinSelect(PinSelectPurpose::Generic_output, F("Reset GPIO"),  F("taskdevicepin2"), P180_RST_PIN);

      sensorTypeHelper_webformLoad_allTypes(event, P180_SENSOR_TYPE_INDEX);

      int sizeSingle = 0;

      for (uint8_t i = P180_START_VTYPE; i < 255; ++i) {
        if (getValueCountFromSensorType(static_cast<Sensor_VType>(i), false) == 1) {
          ++sizeSingle;
        }
      }
      uint8_t singleOptions[sizeSingle];
      uint8_t idx = 0;

      // Build a list of all single-value available value VTypes from PR #5199
      for (uint8_t i = P180_START_VTYPE; i < 255; ++i) {
        if (getValueCountFromSensorType(static_cast<Sensor_VType>(i), false) == 1) {
          singleOptions[idx] = i;
          ++idx;
        }
      }
      String strings[P180_CUSTOM_BUFFER_SIZE];
      LoadCustomTaskSettings(event->TaskIndex, strings, P180_CUSTOM_BUFFER_SIZE, 0);

      addFormTextBox(F("I2C Init Commands"),
                     getPluginCustomArgName(40),
                     strings[P180_BUFFER_ENTRY_INIT],
                     P180_MAX_COMMAND_LENGTH);
      addUnit(strformat(F("%d of %d used"), strings[P180_BUFFER_ENTRY_INIT].length(), P180_MAX_COMMAND_LENGTH));

      addFormTextBox(F("I2C Exit Commands"),
                     getPluginCustomArgName(41),
                     strings[P180_BUFFER_ENTRY_EXIT],
                     P180_MAX_COMMAND_LENGTH);
      addUnit(strformat(F("%d of %d used"), strings[P180_BUFFER_ENTRY_EXIT].length(), P180_MAX_COMMAND_LENGTH));

      for (uint8_t i = 0; i < P180_NR_OUTPUT_VALUES; ++i) {
        // type
        sensorTypeHelper_webformLoad(event, P180_VALUE_OFFSET + i, sizeSingle, singleOptions, false, i + 1);

        // Name
        addFormTextBox(strformat(F("Cache-Name %d (optional)"), i + 1),
                       getPluginCustomArgName(20 + i),
                       strings[P180_BUFFER_START_CACHE + i],
                       P180_MAX_NAME_LENGTH);

        // Add I2C commands sequence
        addFormTextBox(concat(F("I2C Commands "), i + 1),
                       getPluginCustomArgName(30 + i),
                       strings[P180_BUFFER_START_COMMANDS + i],
                       P180_MAX_COMMAND_LENGTH);
        addUnit(strformat(F("%d of %d used"), strings[P180_BUFFER_START_COMMANDS + i].length(), P180_MAX_COMMAND_LENGTH));
      }

      addFormCheckBox(F("Parsing & executing log (INFO)"), F("plog"), P180_LOG_DEBUG == 1);

      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE:
    {
      const String addr = webArg(F("i2c_addr"));
      P180_I2C_ADDRESS = (uint8_t)strtol(addr.c_str(), 0, 16);
      P180_ENABLE_PIN  = getFormItemInt(F("taskdevicepin1"));
      P180_RST_PIN     = getFormItemInt(F("taskdevicepin2"));
      P180_LOG_DEBUG   = isFormItemChecked(F("plog")) ? 1 : 0;
      pconfig_webformSave(event, P180_SENSOR_TYPE_INDEX);

      for (uint8_t i = 0; i < P180_NR_OUTPUT_VALUES; ++i) {
        PCONFIG(P180_VALUE_OFFSET + i) = getFormItemInt(sensorTypeHelper_webformID(P180_VALUE_OFFSET + i));
      }

      {
        String strings[P180_CUSTOM_BUFFER_SIZE];

        for (uint8_t varNr = 0; varNr < VARS_PER_TASK; varNr++) {
          strings[P180_BUFFER_START_CACHE + varNr]    = webArg(getPluginCustomArgName(20 + varNr)); // Name
          strings[P180_BUFFER_START_COMMANDS + varNr] = webArg(getPluginCustomArgName(30 + varNr)); // Commands
        }
        strings[P180_BUFFER_ENTRY_INIT] = webArg(getPluginCustomArgName(40));                       // INIT Commands
        strings[P180_BUFFER_ENTRY_EXIT] = webArg(getPluginCustomArgName(41));                       // EXIT Commands

        const String error = SaveCustomTaskSettings(event->TaskIndex, strings, P180_CUSTOM_BUFFER_SIZE, 0);

        if (!error.isEmpty()) {
          addHtmlError(error);
        }
      }
      success = true;
      break;
    }

    case PLUGIN_INIT:
    {
      initPluginTaskData(event->TaskIndex, new (std::nothrow) P180_data_struct(event));
      P180_data_struct *P180_data = static_cast<P180_data_struct *>(getPluginTaskData(event->TaskIndex));

      if (nullptr != P180_data) {
        success = P180_data->plugin_init(event);
      }

      break;
    }

    case PLUGIN_EXIT:
    {
      P180_data_struct *P180_data = static_cast<P180_data_struct *>(getPluginTaskData(event->TaskIndex));

      if (nullptr != P180_data) {
        P180_data->plugin_exit(event);
      }
      break;
    }

    case PLUGIN_READ:
    {
      P180_data_struct *P180_data = static_cast<P180_data_struct *>(getPluginTaskData(event->TaskIndex));

      if (nullptr != P180_data) {
        success = P180_data->plugin_read(event);
      }
      break;
    }

    case PLUGIN_WRITE:
    {
      P180_data_struct *P180_data = static_cast<P180_data_struct *>(getPluginTaskData(event->TaskIndex));

      if (nullptr != P180_data) {
        success = P180_data->plugin_write(event, string);
      }

      break;
    }

    case PLUGIN_GET_CONFIG_VALUE:
    {
      P180_data_struct *P180_data = static_cast<P180_data_struct *>(getPluginTaskData(event->TaskIndex));

      if (nullptr != P180_data) {
        success = P180_data->plugin_get_config_value(event, string);
      }

      break;
    }
  }
  return success;
}

#endif // USES_P180
