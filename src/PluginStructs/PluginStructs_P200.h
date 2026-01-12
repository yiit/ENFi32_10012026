#ifndef PLUGINSTRUCTS_P200_H
#define PLUGINSTRUCTS_P200_H

/*
   P200 Plugin Structures
   NAU7802 24-bit Scale Plugin
*/

// Plugin configuration structure
struct P200_Config {
  uint8_t i2c_address;
  uint8_t channel;           // 0=Ch1, 1=Ch2, 2=Both
  uint8_t filter_type;       // 0=None, 1=Moving Avg, 2=Exponential
  uint8_t samples;           // Number of samples for averaging
  uint8_t gain;              // 0=x128, 1=x64, 2=x32, 3=x16
  uint8_t sample_rate;       // 0=10Hz, 1=20Hz, 2=40Hz, 3=80Hz
  uint8_t decimal_places;    // 0-6
  uint8_t auto_tare;         // 0=Manual, 1=Auto
};

// Runtime data structure
struct P200_Data {
  P200_Data() : 
    zero_offset(0),
    scale_factor(1000000),
    max_capacity(10000),
    last_raw_value(0),
    filter_sum(0),
    filter_count(0),
    status(0),
    initialized(false),
    last_read_time(0) {}

  int32_t zero_offset;       // Tare/zero calibration offset
  int32_t scale_factor;      // Scale calibration (multiplied by 1M for precision)
  uint32_t max_capacity;     // Maximum capacity in grams
  int32_t last_raw_value;    // Last ADC reading
  int64_t filter_sum;        // For averaging filter
  uint16_t filter_count;     // Sample counter for averaging
  uint8_t status;            // Device status
  boolean initialized;       // Initialization flag
  unsigned long last_read_time;
};

#endif // PLUGINSTRUCTS_P200_H
