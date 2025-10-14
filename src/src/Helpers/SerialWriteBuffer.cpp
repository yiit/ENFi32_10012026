#include "../Helpers/SerialWriteBuffer.h"

#include "../Globals/Logging.h"

#include "../Helpers/ESPEasy_time_calc.h"
#include "../Helpers/Memory.h"
#include "../Helpers/StringConverter.h"

void SerialWriteBuffer_t::clear()
{
  _prefix.clear();
  free_string(_message);
  _timestamp = 0;
  _readpos   = 0;
}

String colorize(const String& str, uint8_t loglevel) {
  const __FlashStringHelper *format = F("%s");

  switch (loglevel)
  {
    case LOG_LEVEL_NONE: format = F("\033[1m%s\033[0m");        // Bold or increased intensity
      break;
    case LOG_LEVEL_INFO: format = F("\033[34m%s\033[0m");       // blue
      break;
    case LOG_LEVEL_ERROR: format = F("\033[91;1m%s\033[0m");    // Red + Bold or increased intensity
      break;
#ifndef BUILD_NO_DEBUG
    case LOG_LEVEL_DEBUG: format = F("\033[32m%s\033[0m");      // Green
      break;
    case LOG_LEVEL_DEBUG_MORE: format = F("\033[35m%s\033[0m"); // Purple
      break;
    case LOG_LEVEL_DEBUG_DEV: format = F("\033[33m%s\033[0m");  // Yellow
      break;
#endif

  }
  return strformat(format, str.c_str());
}

size_t SerialWriteBuffer_t::write(Stream& stream, size_t nrBytesToWrite)
{
  size_t bytesWritten = 0;

  if ((_timestamp != 0) && (timePassedSince(_timestamp) > LOG_BUFFER_EXPIRE)) {
    clear();

    // Mark with empty line we skipped the rest of the message.
    bytesWritten += stream.println(F(" ..."));
    bytesWritten += stream.println();
  }

  if (_timestamp == 0) {
    _readpos = 0;

    // Need to fetch a line
    if (!Logging.getNext(_log_destination, _timestamp, _message, _loglevel) ||
        !loglevelActiveFor(_log_destination, _loglevel)) {
      free_string(_message);
      _timestamp = 0;
      return bytesWritten;
    }

    // Prepare prefix
    _prefix = format_msec_duration(_timestamp);

    if (_loglevel == LOG_LEVEL_NONE) {
      _prefix += F("\033[31;1m :>  \033[0m"); // F(" :>  ");
    } else {
      #ifndef LIMIT_BUILD_SIZE
      _prefix += strformat(F(" : (%d) "), FreeMem());
      #endif
      {
        String loglevelDisplayString = getLogLevelDisplayString(_loglevel);

        while (loglevelDisplayString.length() < LOG_LEVEL_MAX_STRING_LENGTH) {
          loglevelDisplayString += ' ';
        }
        _prefix += colorize(loglevelDisplayString, _loglevel);
      }
      _prefix += F(" | ");
    }
  }

  const size_t maxToWrite = _prefix.length() + _message.length();

  if (nrBytesToWrite > maxToWrite) {
    nrBytesToWrite = maxToWrite;
  }

  bool done = false;

  while (!done && bytesWritten < nrBytesToWrite) {
    if (_readpos < _prefix.length()) {
      // Write prefix
      if (1 != stream.write(_prefix[_readpos])) { return bytesWritten; }
      ++bytesWritten;
      ++_readpos;
    } else if (!_prefix.isEmpty()) {
      // Clear prefix
      _prefix.clear();
      _readpos = 0;
    } else if (_readpos < _message.length()) {
      // Write message
      if (1 != stream.write(_message[_readpos])) { return bytesWritten; }
      ++bytesWritten;
      ++_readpos;
    } else {
      if (1 != stream.write('\n')) { return bytesWritten; }

      // Done with entry, cleanup and leave
      ++bytesWritten;
      clear();
      done = true;
    }
  }
  return bytesWritten;
}
