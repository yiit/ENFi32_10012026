#include "../DataStructs/LogBuffer.h"

#include "../Helpers/ESPEasy_time_calc.h"
#include "../Helpers/StringConverter.h"

void LogBuffer::add(LogEntry_t&& logEntry) {
  clearExpiredEntries();

  if (logEntry) {
    LogEntries.emplace_back(std::move(logEntry));
  }
}

bool LogBuffer::getNext(uint8_t logDestination, uint32_t& timestamp, String& message, uint8_t& loglevel)
{
  if (logDestination == LOG_TO_WEBLOG) {
    lastReadTimeStamp = millis();
  }

  for (LogEntry_queue::iterator it = LogEntries.begin(); it != LogEntries.end(); ++it)
  {
    if (it->validForSubscriber(logDestination)) {
      timestamp = it->getTimestamp();
      message   = it->getMessage();
      loglevel  = it->getLogLevel();
      it->markReadBySubscriber(logDestination);
      clearExpiredEntries();
      return true;
    }
  }
  return false;
}

bool LogBuffer::logActiveRead() {
  clearExpiredEntries();
  return timePassedSince(lastReadTimeStamp) < LOG_BUFFER_ACTIVE_READ_TIMEOUT;
}

void LogBuffer::clearExpiredEntries() {
  for (auto it = LogEntries.begin(); it != LogEntries.end();)
  {
    if (it->isExpired()) {
      it = LogEntries.erase(it);
    } else {
      return;
    }
  }
}
