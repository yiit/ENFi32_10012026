#include "../DataTypes/LogLevels.h"

#include "../DataTypes/FormSelectorOptions.h"

const __FlashStringHelper *logLevelDisplayStrings[] =
{
  F("None"),
  F("Error"),
  F("Info")
#ifndef BUILD_NO_DEBUG
  , F("Debug")
  , F("Debug More")
  , F("Debug dev")
#endif // ifndef BUILD_NO_DEBUG
};

constexpr int logLevels[] =
{
  LOG_LEVEL_NONE,
  LOG_LEVEL_ERROR,
  LOG_LEVEL_INFO

#ifndef BUILD_NO_DEBUG
  , LOG_LEVEL_DEBUG
  , LOG_LEVEL_DEBUG_MORE
  , LOG_LEVEL_DEBUG_DEV
#endif // ifndef BUILD_NO_DEBUG

};

uint8_t                    LOG_LEVEL_NRELEMENTS() { return NR_ELEMENTS(logLevels); }

const __FlashStringHelper* getLogLevelDisplayString(int logLevel) {
  for (uint8_t i = 0; i < NR_ELEMENTS(logLevels); ++i) {
    if (logLevels[i] == logLevel) {
      return logLevelDisplayStrings[logLevel];
    }
  }
  return F("");
}

const __FlashStringHelper* getLogLevelDisplayStringFromIndex(uint8_t index, int& logLevel) {
  if (index < NR_ELEMENTS(logLevels)) {
    logLevel = logLevels[index];
    return logLevelDisplayStrings[index];
  }
  logLevel = -1;
  return F("");
}

void addLogLevelFormSelectorOptions(const String& id, int choice)
{
  const FormSelectorOptions selector(NR_ELEMENTS(logLevels), logLevelDisplayStrings, logLevels);

  selector.addSelector(id, choice);
}
