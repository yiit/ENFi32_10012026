#ifndef HELPERS_SERIALWRITEBUFFER_H
#define HELPERS_SERIALWRITEBUFFER_H

#include "../../ESPEasy_common.h"

#include <deque>

#ifndef MAX_SERIALWRITEBUFFER_SIZE
# ifdef ESP8266
#  define MAX_SERIALWRITEBUFFER_SIZE 1024
# endif // ifdef ESP8266
# ifdef ESP32
#  define MAX_SERIALWRITEBUFFER_SIZE 10240
# endif // ifdef ESP32
#endif // ifndef MAX_SERIALWRITEBUFFER_SIZE

class SerialWriteBuffer_t {
public:

  SerialWriteBuffer_t(uint8_t log_destination) : _log_destination(log_destination) {}

  void   clear();

  size_t write(Stream& stream,
               size_t  nrBytesToWrite);

private:

  String _prefix;
  String _message;
  uint32_t _timestamp{};
  uint32_t _readpos{};
  uint8_t _loglevel{};

  const uint8_t _log_destination;
};

#endif // ifndef HELPERS_SERIALWRITEBUFFER_H
