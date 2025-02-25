#include "../PluginStructs/P177_data_struct.h"

#ifdef USES_P177


P177_data_struct::P177_data_struct(struct EventStruct *event) {
  _sendEvents     = P177_GENERATE_EVENTS == 1;
  _rawData        = P177_RAW_DATA == 1;
  _ignoreNegative = P177_IGNORE_NEGATIVE == 1;
}

P177_data_struct::~P177_data_struct() {}

bool P177_data_struct::plugin_ten_per_second(struct EventStruct *event) {
  bool is_ok;
  uint8_t byteData;

  if (P177_SensorMode_e::IdleMode == _sensorMode) {
    _cycles--;

    if (_cycles) { // Skip until counted down to 0
      return false;
    }
    byteData = I2C_read8_reg(P177_I2C_ADDR, P177_COMMAND_REG, &is_ok);

    if (is_ok && (0 == bitRead(byteData, 3))) { // Not currently reading
      I2C_write8_reg(P177_I2C_ADDR, P177_COMMAND_REG, P177_START_READ);
      _sensorMode = P177_SensorMode_e::SensingMode;
      _cycles     = P177_SKIP_CYCLES;           // Restart timer after a read is started
    }
    return false;
  }

  if (P177_SensorMode_e::SensingMode == _sensorMode) {
    byteData = I2C_read8_reg(P177_I2C_ADDR, P177_COMMAND_REG, &is_ok);

    if (!is_ok || (1 == bitRead(byteData, 3))) { // Currently reading
      return false;
    }
    _sensorMode = P177_SensorMode_e::ReportingMode;
  }

  uint32_t prData{};
  uint32_t tmData{};

  # if P177_LOG
  String log = F("P177 : RAW data: ");
  # endif // if P177_LOG
  size_t i = 0;

  for (; i < 3; ++i) { // first 3 bytes pressure
    prData <<= 8;
    byteData = I2C_read8_reg(P177_I2C_ADDR, P177_DATA_REG_1 + i);
    prData  += byteData;
    # if P177_LOG
    log += strformat(F("0x%02X "), byteData);
    # endif // if P177_LOG
  }
  # if P177_LOG
  log += strformat(F("prData: 0x%06X, "), prData);
  # endif // if P177_LOG

  for (; i < P177_DATA_BYTES; ++i) { // 2 more bytes temperature
    tmData <<= 8;
    byteData = I2C_read8_reg(P177_I2C_ADDR, P177_DATA_REG_1 + i);
    tmData  += byteData;
    # if P177_LOG
    log += strformat(F("0x%02X "), byteData);
    # endif // if P177_LOG
  }

  # if P177_LOG
  log += strformat(F(" tmData: 0x%04X"), tmData);
  addLog(LOG_LEVEL_INFO, log);
  # endif // if P177_LOG

  _sensorMode = P177_SensorMode_e::IdleMode; // Start a new cycle
  const bool pressureChanged = (prData != _rawPressure);

  if (!pressureChanged && (tmData == _rawTemperature)) {
    return false;
  }
  _rawPressure    = prData;
  _rawTemperature = tmData;
  _updated        = true;

  if (_sendEvents && pressureChanged) {
    plugin_read(event);
    sendData(event);
    _updated = true; // Is reset in plugin_read()
  }
  return true;
}

bool P177_data_struct::plugin_read(struct EventStruct *event) {
  if (!_updated) {
    // Needed to get the 1st value, or can get the latest value when sampled too frequently.
    plugin_ten_per_second(event);
  }

  if (_updated) {
    int tmpPressure                       = _rawPressure; // Unsigned to signed
    int tmpTemperature                    = _rawTemperature;
    ESPEASY_RULES_FLOAT_TYPE _pressure    = 1.0 * tmpPressure;
    ESPEASY_RULES_FLOAT_TYPE _temperature = 1.0 * tmpTemperature;

    if (tmpPressure > (1 << 23)) { // Negative value
      if (_ignoreNegative) {
        _pressure = 0.0;
      } else {
        _pressure = 1.0 * (tmpPressure - (1 << 24));
      }
    }

    if (tmpTemperature > (1 << 15)) { // Negative value?
      _temperature = 1.0 * (tmpTemperature - (1 << 15));
    }

    if (!essentiallyZero(_temperature)) {
      _temperature /= 256.0; // Move less significant bits
    }

    if (!_rawData) {
      if (!essentiallyZero(_pressure)) {
        _pressure /= (1.0 * ((1 << 23) / P177_PRESSURE_SCALE_FACTOR));
      }

      if (P177_TEMPERATURE_OFFSET != 0) {
        _temperature += (P177_TEMPERATURE_OFFSET / 10.0); // In 0.1 degree steps
      }
    }

    UserVar.setFloat(event->TaskIndex, 0, _pressure);
    UserVar.setFloat(event->TaskIndex, 1, _temperature);
    _updated = false;
    return true;
  }
  return false;
}

#endif // ifdef USES_P177
