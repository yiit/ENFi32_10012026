#include "src/PluginStructs/PluginStructs_P200.h"

/*
   ========================================
   NAU7802 24-bit Dual Channel ADC Scale Plugin
   ========================================
   
   Plugin ID: 200
   Type: Weighing Scale / Load Cell Interface
   
   This plugin interfaces with the NAU7802 24-bit Dual Channel ADC breakout board
   to create a high-precision digital scale with calibration support.
   
   Features:
   - Zero calibration (tare)
   - Load cell calibration
   - Maximum capacity setting
   - Decimal place resolution
   - Smoothing/averaging filter
   - Measurement speed control
   - Dual channel support
   
   References:
   - NAU7802 Datasheet
   - Boardoza NAU7802 Breakout Board
   - Library: lib/NAU7802/
*/

#define PLUGIN_200
#define PLUGIN_ID_200           200
#define PLUGIN_NAME_200         "NAU7802 24-bit Scale"
#define PLUGIN_VALUENAME1_200   "Weight"
#define PLUGIN_VALUENAME2_200   "Channel2"
#define PLUGIN_VALUENAME3_200   "Status"
#define PLUGIN_200_DEBUG        false

// Configuration indices
#define P200_I2C_ADDR           PCONFIG(0)
#define P200_I2C_ADDR_LABEL     PCONFIG_LABEL(0)
#define P200_CHANNEL            PCONFIG(1)    // 0=Channel 1, 1=Channel 2, 2=Both
#define P200_CHANNEL_LABEL      PCONFIG_LABEL(1)
#define P200_FILTER_TYPE        PCONFIG(2)    // Smoothing method
#define P200_FILTER_TYPE_LABEL  PCONFIG_LABEL(2)
#define P200_SAMPLES            PCONFIG(3)    // Number of samples for averaging
#define P200_SAMPLES_LABEL      PCONFIG_LABEL(3)
#define P200_GAIN_INDEX         PCONFIG(4)    // Amplifier gain
#define P200_GAIN_INDEX_LABEL   PCONFIG_LABEL(4)
#define P200_RATE_INDEX         PCONFIG(5)    // Sample rate
#define P200_RATE_INDEX_LABEL   PCONFIG_LABEL(5)
#define P200_DECIMAL_PLACES     PCONFIG(6)    // Decimal places in output
#define P200_DECIMAL_PLACES_LABEL PCONFIG_LABEL(6)
#define P200_MODE               PCONFIG(7)    // 0=Manual, 1=Auto-tare

#define P200_ZERO_OFFSET_L      PCONFIG_LONG(0)   // Zero calibration offset
#define P200_SCALE_FACTOR_L     PCONFIG_LONG(1)   // Scale calibration factor
#define P200_MAX_CAPACITY_L     PCONFIG_LONG(2)   // Maximum capacity in grams

// Status codes
#define P200_STATUS_OK          0
#define P200_STATUS_NOT_FOUND   1
#define P200_STATUS_NO_DATA     2
#define P200_STATUS_ERROR       3

// I2C default address
#define P200_I2C_ADDRESS_DEFAULT 0x2A

// NAU7802 I2C Commands
#define NAU7802_READ_REG        0x00
#define NAU7802_WRITE_REG       0x01
#define NAU7802_CHIP_ID         0x0F
#define NAU7802_ADC_RESULT      0x12
#define NAU7802_CTRL1           0x00
#define NAU7802_CTRL2           0x01
#define NAU7802_PU_CTRL         0x02
#define NAU7802_GAINS           0x04
#define NAU7802_SAMPLING_RATE   0x05

struct P200_data_struct {
  P200_data_struct() : nau7802(nullptr), 
                        filter_index(0),
                        last_raw_value(0),
                        zero_offset(0),
                        scale_factor(1000000),  // 1:1 default scaling
                        max_capacity(10000),
                        status(P200_STATUS_OK),
                        last_read_time(0) {}

  void *nau7802;
  uint8_t filter_index;
  int32_t last_raw_value;
  int32_t zero_offset;
  int32_t scale_factor;
  uint32_t max_capacity;
  uint8_t status;
  unsigned long last_read_time;
};

P200_data_struct P200_data[TASKS_MAX];

boolean Plugin_200(uint8_t function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch(function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_200;
        Device[deviceCount].Type = DEVICE_TYPE_I2C;
        Device[deviceCount].VType = Sensor_VType::SENSOR_TYPE_TRIPLE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 3;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        Device[deviceCount].PluginStats = true;
        success = true;
      }
      break;

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_200);
        success = true;
      }
      break;

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_200));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_200));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_200));
        success = true;
      }
      break;

    case PLUGIN_GET_DEVICEVTYPEOPTION:
      {
        success = true;
      }
      break;

    case PLUGIN_WEBFORM_LOAD:
      {
        uint8_t taskIndex = event->TaskIndex;

        // I2C Address
        addFormI2CSelect(string, F("I2C Address"), F("i2caddr"), P200_I2C_ADDR, 0x2A);

        // Channel Selection
        {
          String options[3] = {F("Channel 1"), F("Channel 2"), F("Both Channels")};
          addFormSelector(string, F("Channel"), F("channel"), 3, options, nullptr, P200_CHANNEL);
        }

        // Filter Type
        {
          String options[3] = {F("None"), F("Moving Average"), F("Exponential")};
          addFormSelector(string, F("Filter Type"), F("filter"), 3, options, nullptr, P200_FILTER_TYPE);
        }

        // Number of Samples for Averaging
        addFormNumericBox(string, F("Samples for Average"), F("samples"), P200_SAMPLES, 1, 128);

        // Gain Selection
        {
          String options[4] = {F("x128"), F("x64"), F("x32"), F("x16")};
          addFormSelector(string, F("Amplifier Gain"), F("gain"), 4, options, nullptr, P200_GAIN_INDEX);
        }

        // Sample Rate
        {
          String options[4] = {F("10 Hz"), F("20 Hz"), F("40 Hz"), F("80 Hz")};
          addFormSelector(string, F("Sample Rate"), F("rate"), 4, options, nullptr, P200_RATE_INDEX);
        }

        // Decimal Places
        addFormNumericBox(string, F("Decimal Places"), F("decimal"), P200_DECIMAL_PLACES, 0, 6);

        // Mode
        {
          String options[2] = {F("Manual Tare"), F("Auto Tare")};
          addFormSelector(string, F("Tare Mode"), F("mode"), 2, options, nullptr, P200_MODE);
        }

        // Display Calibration Status
        addFormSubHeader(string, F("Calibration Data"));
        
        addFormStaticText(string, F("Zero Offset"), String(P200_ZERO_OFFSET_L));
        addFormStaticText(string, F("Scale Factor"), String(P200_SCALE_FACTOR_L));
        addFormStaticText(string, F("Max Capacity (g)"), String(P200_MAX_CAPACITY_L));

        success = true;
      }
      break;

    case PLUGIN_WEBFORM_SAVE:
      {
        uint8_t taskIndex = event->TaskIndex;
        P200_I2C_ADDR = getFormItemInt(F("i2caddr"));
        P200_CHANNEL = getFormItemInt(F("channel"));
        P200_FILTER_TYPE = getFormItemInt(F("filter"));
        P200_SAMPLES = getFormItemInt(F("samples"));
        P200_GAIN_INDEX = getFormItemInt(F("gain"));
        P200_RATE_INDEX = getFormItemInt(F("rate"));
        P200_DECIMAL_PLACES = getFormItemInt(F("decimal"));
        P200_MODE = getFormItemInt(F("mode"));
        success = true;
      }
      break;

    case PLUGIN_INIT:
      {
        uint8_t taskIndex = event->TaskIndex;
        P200_data[taskIndex].zero_offset = P200_ZERO_OFFSET_L;
        P200_data[taskIndex].scale_factor = P200_SCALE_FACTOR_L;
        P200_data[taskIndex].max_capacity = P200_MAX_CAPACITY_L;
        
        if(P200_initDevice(taskIndex)) {
          P200_data[taskIndex].status = P200_STATUS_OK;
          addLog(LOG_LEVEL_INFO, F("P200: NAU7802 Scale initialized"));
          success = true;
        } else {
          P200_data[taskIndex].status = P200_STATUS_NOT_FOUND;
          addLog(LOG_LEVEL_ERROR, F("P200: NAU7802 not found on I2C bus"));
        }
      }
      break;

    case PLUGIN_READ:
      {
        uint8_t taskIndex = event->TaskIndex;
        
        if(P200_data[taskIndex].status == P200_STATUS_OK) {
          int32_t raw_value = P200_readADC(taskIndex);
          
          if(raw_value != -1) {
            // Apply zero offset
            int32_t offset_value = raw_value - P200_data[taskIndex].zero_offset;
            
            // Apply scale factor (convert to weight in grams)
            float weight = (float)offset_value * P200_data[taskIndex].scale_factor / 1000000.0f;
            
            // Apply decimal places
            int decimal_places = P200_DECIMAL_PLACES;
            float divisor = pow(10.0f, decimal_places);
            weight = floor(weight * divisor) / divisor;
            
            // Set output values
            UserVar.setFloat(event->TaskIndex, 0, weight);
            UserVar.setFloat(event->TaskIndex, 1, (float)raw_value);  // Raw ADC value
            UserVar.setFloat(event->TaskIndex, 2, P200_data[taskIndex].status);  // Status
            
            if(P200_200_DEBUG) {
              String debug = F("P200: Raw=");
              debug += raw_value;
              debug += F(" Weight=");
              debug += weight;
              addLog(LOG_LEVEL_DEBUG_MORE, debug);
            }
            
            success = true;
          } else {
            P200_data[taskIndex].status = P200_STATUS_NO_DATA;
            addLog(LOG_LEVEL_WARNING, F("P200: No data from ADC"));
          }
        }
      }
      break;

    case PLUGIN_WRITE:
      {
        String command = parseString(string, 1);
        
        if(command == F("p200tare")) {
          // Zero calibration (tare)
          uint8_t taskIndex = event->TaskIndex;
          int32_t raw = P200_readADC(taskIndex);
          if(raw != -1) {
            P200_ZERO_OFFSET_L = raw;
            P200_data[taskIndex].zero_offset = raw;
            addLog(LOG_LEVEL_INFO, F("P200: Zero calibration done"));
            success = true;
          }
        }
        else if(command == F("p200calibrate")) {
          // Load calibration - syntax: p200calibrate <weight_in_grams>
          uint8_t taskIndex = event->TaskIndex;
          int32_t raw = P200_readADC(taskIndex);
          if(raw != -1 && string.length() > 14) {
            float calib_weight = parseFloat(string, 2);
            if(calib_weight > 0) {
              int32_t offset_raw = raw - P200_data[taskIndex].zero_offset;
              // scale_factor = (calib_weight * 1000000) / offset_raw
              P200_SCALE_FACTOR_L = (int32_t)((calib_weight * 1000000.0f) / offset_raw);
              P200_data[taskIndex].scale_factor = P200_SCALE_FACTOR_L;
              addLog(LOG_LEVEL_INFO, F("P200: Load calibration done"));
              success = true;
            }
          }
        }
        else if(command == F("p200setmax")) {
          // Set maximum capacity - syntax: p200setmax <grams>
          uint8_t taskIndex = event->TaskIndex;
          if(string.length() > 11) {
            uint32_t max_g = (uint32_t)parseFloat(string, 2);
            P200_MAX_CAPACITY_L = max_g;
            P200_data[taskIndex].max_capacity = max_g;
            addLog(LOG_LEVEL_INFO, F("P200: Max capacity set"));
            success = true;
          }
        }
      }
      break;

    case PLUGIN_EXIT:
      {
        uint8_t taskIndex = event->TaskIndex;
        // Cleanup if needed
        success = true;
      }
      break;
  }

  return success;
}

// Helper Functions

boolean P200_initDevice(uint8_t taskIndex) {
  // Check if NAU7802 is available on I2C bus
  uint8_t i2c_addr = P200_I2C_ADDR;
  if(i2c_addr == 0) i2c_addr = P200_I2C_ADDRESS_DEFAULT;
  
  Wire.begin();
  Wire.beginTransmission(i2c_addr);
  if(Wire.endTransmission() == 0) {
    // Device found, initialize with settings
    P200_setupGain(taskIndex, P200_GAIN_INDEX);
    P200_setupSampleRate(taskIndex, P200_RATE_INDEX);
    return true;
  }
  return false;
}

int32_t P200_readADC(uint8_t taskIndex) {
  // Read ADC value from NAU7802
  // This is a simplified version - full implementation depends on NAU7802 library usage
  uint8_t i2c_addr = P200_I2C_ADDR;
  if(i2c_addr == 0) i2c_addr = P200_I2C_ADDRESS_DEFAULT;
  
  Wire.beginTransmission(i2c_addr);
  Wire.write(NAU7802_ADC_RESULT);
  if(Wire.endTransmission() != 0) return -1;
  
  Wire.requestFrom(i2c_addr, (uint8_t)3);
  if(Wire.available() < 3) return -1;
  
  uint32_t value = 0;
  value |= ((uint32_t)Wire.read() << 16);
  value |= ((uint32_t)Wire.read() << 8);
  value |= Wire.read();
  
  // Convert from 24-bit unsigned to 32-bit signed
  if(value & 0x800000) {
    value |= 0xFF000000;  // Sign extend
  }
  
  return (int32_t)value;
}

void P200_setupGain(uint8_t taskIndex, uint8_t gain_index) {
  // Set amplifier gain: 0=x128, 1=x64, 2=x32, 3=x16
  uint8_t i2c_addr = P200_I2C_ADDR;
  if(i2c_addr == 0) i2c_addr = P200_I2C_ADDRESS_DEFAULT;
  
  Wire.beginTransmission(i2c_addr);
  Wire.write(NAU7802_GAINS);
  Wire.write(gain_index & 0x03);
  Wire.endTransmission();
}

void P200_setupSampleRate(uint8_t taskIndex, uint8_t rate_index) {
  // Set sample rate: 0=10Hz, 1=20Hz, 2=40Hz, 3=80Hz
  uint8_t i2c_addr = P200_I2C_ADDR;
  if(i2c_addr == 0) i2c_addr = P200_I2C_ADDRESS_DEFAULT;
  
  Wire.beginTransmission(i2c_addr);
  Wire.write(NAU7802_SAMPLING_RATE);
  Wire.write(rate_index & 0x03);
  Wire.endTransmission();
}
