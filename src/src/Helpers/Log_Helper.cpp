#include "../Helpers/Log_Helper.h"


#include "../Globals/ESPEasy_Console.h"
#include "../Helpers/Networking.h"
#include "../Helpers/StringConverter.h"


#include <FS.h>

#if FEATURE_SD
# include <SD.h>
# include "../Helpers/ESPEasy_Storage.h"
#endif // if FEATURE_SD

void addToSerialLog(uint32_t timestamp, const String& message, uint8_t loglevel)
{
  if (loglevelActiveFor(LOG_TO_SERIAL, loglevel)) {
    ESPEasy_Console.addToSerialBuffer(format_msec_duration(timestamp));
    #ifndef LIMIT_BUILD_SIZE
    ESPEasy_Console.addToSerialBuffer(strformat(F(" : (%d) "), FreeMem()));
    #endif
    {
      String loglevelDisplayString = getLogLevelDisplayString(loglevel);

      while (loglevelDisplayString.length() < 6) {
        loglevelDisplayString += ' ';
      }
      ESPEasy_Console.addToSerialBuffer(loglevelDisplayString);
    }
    ESPEasy_Console.addToSerialBuffer(F(" : "));
    ESPEasy_Console.addToSerialBuffer(message);
    ESPEasy_Console.addNewlineToSerialBuffer();
  }
}

void addToSysLog(uint8_t logLevel, const String& str)
{
  if (loglevelActiveFor(LOG_TO_SYSLOG, logLevel)) {
    sendSyslog(logLevel, str);
  }
}

void addToSDLog(uint8_t logLevel, const String& str)
{
#if FEATURE_SD

  if (!str.isEmpty() && loglevelActiveFor(LOG_TO_SDCARD, logLevel)) {
    String   logName = patch_fname(F("log.txt"));
    fs::File logFile = SD.open(logName, "a+");

    if (logFile) {
      const size_t stringLength = str.length();

      for (size_t i = 0; i < stringLength; ++i) {
        logFile.print(str[i]);
      }
      logFile.println();
    }
    logFile.close();
  }
#endif // if FEATURE_SD
}

void LogHelper::addLogEntry(LogEntry_t&& logEntry)
{
      #ifdef ESP32

  if (xPortInIsrContext()) {
    // When called from an ISR, you should not send out logs.
    // Allocating memory from within an ISR is a big no-no.
    // Also long-time blocking like sending logs (especially to a syslog server)
    // is also really not a good idea from an ISR call.
    return;
  }
  #endif // ifdef ESP32

  logEntry.setSubscribers();

  if (!logEntry) { return; }

  _logBuffer.add(std::move(logEntry));

  loop();
}

bool LogHelper::getNext(uint8_t logDestination, uint32_t& timestamp, String& message, uint8_t& loglevel)
{
  return _logBuffer.getNext(logDestination, timestamp, message, loglevel);
}

void LogHelper::loop()             {
  const uint8_t destinations[] = {
    // TODO TD-er: Add log buffer for 2nd serial console
    LOG_TO_SERIAL,
    LOG_TO_SYSLOG,
    LOG_TO_SDCARD
  };

  for (int i = 0; i < NR_ELEMENTS(destinations); ++i) {
    String   message;
    uint32_t timestamp{};
    uint8_t  loglevel{};

    if (_logBuffer.getNext(destinations[i], timestamp, message, loglevel))
    {
      switch (destinations[i])
      {
        case LOG_TO_SERIAL:
          addToSerialLog(timestamp, message, loglevel);
          break;
        case LOG_TO_SYSLOG:
          addToSysLog(loglevel, message);
          break;
        case LOG_TO_SDCARD:
          addToSDLog(loglevel, message);
          break;
      }
    }
  }

  _logBuffer.clearExpiredEntries();
}

bool LogHelper::webLogActiveRead() { return _logBuffer.logActiveRead(); }
