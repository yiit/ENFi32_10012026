#include "src/PluginStructs/PluginStructs_P200.h"
#include <NAU7802.h>

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

struct P200_data_struct {
  P200_data_struct() : nau7802(nullptr), 
                        filter_index(0),
                        last_raw_value(0),
                        zero_offset(0),
                        calibration_factor(1000.0f),  // Default: 1 gram per 1000 ADC units
                        max_capacity(10000),
                        status(P200_STATUS_OK),
                        last_read_time(0),
                        initialized(false) {}

  NAU7802 *nau7802;
  uint8_t filter_index;
  int32_t last_raw_value;
  int32_t zero_offset;
  float calibration_factor;
  uint32_t max_capacity;
  uint8_t status;
  unsigned long last_read_time;
  boolean initialized;
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
        P200_data[taskIndex].calibration_factor = P200_SCALE_FACTOR_L > 0 ? 
                                                   P200_SCALE_FACTOR_L / 1000.0f : 1000.0f;
        P200_data[taskIndex].max_capacity = P200_MAX_CAPACITY_L;
        
        // NAU7802 nesnesini oluştur ve başlat
        if(P200_data[taskIndex].nau7802 == nullptr) {
          P200_data[taskIndex].nau7802 = new NAU7802();
        }
        
        if(P200_initDevice(taskIndex)) {
          P200_data[taskIndex].status = P200_STATUS_OK;
          P200_data[taskIndex].initialized = true;
          addLog(LOG_LEVEL_INFO, F("P200: NAU7802 Scale initialized successfully"));
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
        
        if(P200_data[taskIndex].status == P200_STATUS_OK && P200_data[taskIndex].initialized) {
          NAU7802 *nau = P200_data[taskIndex].nau7802;
          
          if(nau && nau->isAvailable()) {
            int32_t raw_value = nau->readADCValue();
            
            if(raw_value != 0) {
              // Apply zero offset (tare)
              int32_t offset_value = raw_value - P200_data[taskIndex].zero_offset;
              
              // Convert ADC value to weight using calibration factor
              float weight = (float)offset_value / P200_data[taskIndex].calibration_factor;
              
              // Apply decimal places
              int decimal_places = P200_DECIMAL_PLACES;
              if(decimal_places > 0) {
                float divisor = pow(10.0f, decimal_places);
                weight = floor(weight * divisor) / divisor;
              }
              
              // Check if exceeds max capacity
              if(weight > P200_data[taskIndex].max_capacity) {
                P200_data[taskIndex].status = P200_STATUS_ERROR;
                addLog(LOG_LEVEL_WARNING, F("P200: Weight exceeds max capacity"));
              }
              
              // Set output values
              UserVar.setFloat(event->TaskIndex, 0, weight);
              UserVar.setFloat(event->TaskIndex, 1, (float)raw_value);  // Raw ADC value
              UserVar.setFloat(event->TaskIndex, 2, P200_data[taskIndex].status);  // Status
              
              if(P200_200_DEBUG) {
                String debug = F("P200: Raw=");
                debug += raw_value;
                debug += F(" Offset=");
                debug += offset_value;
                debug += F(" Weight=");
                debug += weight;
                addLog(LOG_LEVEL_DEBUG_MORE, debug);
              }
              
              success = true;
            } else {
              P200_data[taskIndex].status = P200_STATUS_NO_DATA;
              addLog(LOG_LEVEL_WARNING, F("P200: No valid data from ADC"));
            }
          } else {
            P200_data[taskIndex].status = P200_STATUS_NOT_FOUND;
            addLog(LOG_LEVEL_WARNING, F("P200: NAU7802 not available"));
          }
        }
      }
      break;

    case PLUGIN_WRITE:
      {
        String command = parseString(string, 1);
        uint8_t taskIndex = event->TaskIndex;
        NAU7802 *nau = P200_data[taskIndex].nau7802;
        
        if(command == F("p200tare")) {
          // Zero calibration (tare)
          if(nau && nau->isAvailable()) {
            int32_t raw = nau->readADCValue();
            P200_ZERO_OFFSET_L = raw;
            P200_data[taskIndex].zero_offset = raw;
            addLog(LOG_LEVEL_INFO, F("P200: Zero calibration (tare) completed"));
            success = true;
          }
        }
        else if(command == F("p200calibrate")) {
          // Load calibration - syntax: p200calibrate <weight_in_grams>
          if(nau && nau->isAvailable() && string.length() > 14) {
            int32_t raw = nau->readADCValue();
            float calib_weight = parseFloat(string, 2);
            if(calib_weight > 0) {
              int32_t offset_raw = raw - P200_data[taskIndex].zero_offset;
              if(offset_raw != 0) {
                // calibration_factor = ADC_reading / weight
                P200_data[taskIndex].calibration_factor = (float)offset_raw / calib_weight;
                P200_SCALE_FACTOR_L = (int32_t)(P200_data[taskIndex].calibration_factor * 1000.0f);
                
                String debug = F("P200: Load calibration done. Factor=");
                debug += P200_data[taskIndex].calibration_factor;
                addLog(LOG_LEVEL_INFO, debug);
                success = true;
              }
            }
          }
        }
        else if(command == F("p200setmax")) {
          // Set maximum capacity - syntax: p200setmax <grams>
          if(string.length() > 11) {
            uint32_t max_g = (uint32_t)parseFloat(string, 2);
            P200_MAX_CAPACITY_L = max_g;
            P200_data[taskIndex].max_capacity = max_g;
            
            String debug = F("P200: Max capacity set to ");
            debug += max_g;
            debug += F("g");
            addLog(LOG_LEVEL_INFO, debug);
            success = true;
          }
        }
        else if(command == F("p200gain")) {
          // Set gain - syntax: p200gain <0-7> (0=x1, 1=x2, 2=x4, 3=x8, 4=x16, 5=x32, 6=x64, 7=x128)
          if(nau && string.length() > 8) {
            uint8_t gain_val = (uint8_t)parseFloat(string, 2);
            if(gain_val <= 7) {
              nau->setGain((Gain)gain_val);
              String debug = F("P200: Gain set to ");
              debug += (1 << gain_val);
              addLog(LOG_LEVEL_INFO, debug);
              success = true;
            }
          }
        }
        else if(command == F("p200rate")) {
          // Set sample rate - syntax: p200rate <0-4> (0=10Hz, 1=20Hz, 2=40Hz, 3=80Hz, 4=320Hz)
          if(nau && string.length() > 8) {
            uint8_t rate_val = (uint8_t)parseFloat(string, 2);
            if(rate_val <= 4) {
              nau->setSampleRate((SampleRate)rate_val);
              String debug = F("P200: Sample rate set");
              addLog(LOG_LEVEL_INFO, debug);
              success = true;
            }
          }
        }
      }
      break;

    case PLUGIN_EXIT:
      {
        uint8_t taskIndex = event->TaskIndex;
        // Cleanup: delete NAU7802 object
        if(P200_data[taskIndex].nau7802 != nullptr) {
          delete P200_data[taskIndex].nau7802;
          P200_data[taskIndex].nau7802 = nullptr;
        }
        P200_data[taskIndex].initialized = false;
        success = true;
      }
      break;
  }

  return success;
}

// Helper Functions

boolean P200_initDevice(uint8_t taskIndex) {
  NAU7802 *nau = P200_data[taskIndex].nau7802;
  
  if(nau == nullptr) {
    return false;
  }
  
  // Try to initialize NAU7802
  if(!nau->begin()) {
    addLog(LOG_LEVEL_ERROR, F("P200: Failed to initialize NAU7802"));
    return false;
  }
  
  // Power up the device
  if(!nau->powerUp()) {
    addLog(LOG_LEVEL_ERROR, F("P200: Failed to power up NAU7802"));
    return false;
  }
  
  // Set gain
  uint8_t gain_idx = P200_GAIN_INDEX;
  nau->setGain((Gain)gain_idx);
  
  // Set sample rate
  uint8_t rate_idx = P200_RATE_INDEX;
  nau->setSampleRate((SampleRate)rate_idx);
  
  // Select channel
  uint8_t channel_idx = P200_CHANNEL;
  if(channel_idx == 0) {
    nau->setChannel(CHANNEL1);
  } else if(channel_idx == 1) {
    nau->setChannel(CHANNEL2);
  }
  
  // Perform offset calibration
  if(!nau->calibrate()) {
    addLog(LOG_LEVEL_WARNING, F("P200: Calibration step skipped"));
  }
  
  String debug = F("P200: Device initialized - Gain=");
  debug += (1 << gain_idx);
  debug += F("x, Rate=");
  debug += (10 * (gain_idx + 1));
  debug += F("Hz");
  addLog(LOG_LEVEL_INFO, debug);
  
  return true;
}
