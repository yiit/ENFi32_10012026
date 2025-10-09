#pragma once

#include "../../ESPEasy_common.h"

#include "../DataStructs/LogBuffer.h"

#include "../DataTypes/LogLevels.h"

class LogHelper
{
public:

  LogHelper() {}

  void addLogEntry(LogEntry_t&& logEntry);

  bool getNext(uint8_t   logDestination,
               uint32_t& timestamp,
               String  & message,
               uint8_t & loglevel);

  void loop();

  bool webLogActiveRead();

private:

  LogBuffer _logBuffer{};
}; // class LogHelper
