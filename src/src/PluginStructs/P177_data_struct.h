#ifndef PLUGINSTRUCTS_P177_DATA_STRUCT_H
#define PLUGINSTRUCTS_P177_DATA_STRUCT_H

#include "../../_Plugin_Helper.h"

#ifdef USES_P177
# include "../Helpers/I2C_access.h"

# define P177_LOG 1              // Set to 1 to enable raw data logging at INFO level

# define P177_I2C_ADDR      0x7F // Fixed address

# define P177_COMMAND_REG   0x30
# define P177_START_READ    0x0A
# define P177_DATA_REG_1    0x06
# define P177_DATA_BYTES    5

# define P177_SKIP_CYCLES   5                       // Skip this number of 10/sec cycles when in IdleMode

# define P177_PRESSURE_SCALE_FACTOR PCONFIG_LONG(0) // Pressure scaling factor
# define P177_TEMPERATURE_OFFSET    PCONFIG(0)      // Temperature compensation in 0.1 degree steps

enum class P177_SensorMode_e : uint8_t {
  IdleMode      = 0u,
  SensingMode   = 1u,
  ReportingMode = 2u,
};

/*******************************************
 * P177 Plugin taskdata struct
 ******************************************/
struct P177_data_struct : public PluginTaskData_base {
public:

  P177_data_struct() = default;
  ~P177_data_struct();

  bool plugin_ten_per_second(struct EventStruct *event);

  bool plugin_read(struct EventStruct *event);

  uint32_t          _rawPressure{};
  uint16_t          _rawTemperature{};
  P177_SensorMode_e _sensorMode = P177_SensorMode_e::IdleMode;
  uint8_t           _cycles     = 1; // Start cycle immediately
  bool              _updated    = false;
};

#endif // ifdef USES_P177
#endif // ifndef PLUGINSTRUCTS_P177_DATA_STRUCT_H
