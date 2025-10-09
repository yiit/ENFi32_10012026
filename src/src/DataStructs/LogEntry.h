#pragma once

#include "../../ESPEasy_common.h"

#include "../DataTypes/LogLevels.h"

#include <memory>

struct LogEntry_t {

  LogEntry_t() = delete;

  LogEntry_t(const LogEntry_t& rhs) = delete;

  LogEntry_t(const uint8_t              logLevel,
             const __FlashStringHelper *message);

  LogEntry_t(const uint8_t logLevel,
             const char   *message);

  LogEntry_t(const uint8_t logLevel,
             const String& message);

  LogEntry_t(const uint8_t logLevel,
             String     && message);

  ~LogEntry_t();

  LogEntry_t(LogEntry_t&& rhs);
  LogEntry_t& operator=(LogEntry_t&& rhs);

  void        clear();

  LogEntry_t& operator=(const LogEntry_t& rhs) = delete;

  operator bool() const { return isValid(); }

  void        setSubscribers();

  void        markReadBySubscriber(uint8_t subscriber);

  bool        validForSubscriber(uint8_t subscriber) const;

  size_t      print(Print& out,
                    size_t offset = 0,
                    size_t length = std::numeric_limits<size_t>::max()) const;

  String   getMessage() const;

  bool     isExpired() const;

  bool     isValid() const;

  uint8_t  getLogLevel() const;

  uint32_t getTimestamp() const { return _timestamp; }

private:

  void    *_message{};
  size_t   _strLength;
  uint32_t _timestamp;
  union {
    struct {
      uint32_t _isFlashString         : 1;
      uint32_t _logLevel              : 4;
      uint32_t _subscriberPendingRead : 8;

      uint32_t unused : 19;

    };

    uint32_t _flags{};

  };

};


typedef std::unique_ptr<LogEntry_t>Up_LogEntry;
typedef std::shared_ptr<LogEntry_t>Sp_LogEntry;
