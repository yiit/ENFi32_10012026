#ifndef GLOBALS_LOGGING_H
#define GLOBALS_LOGGING_H

#include <stdint.h>
#include <deque>

#include "../Helpers/Log_Helper.h"


extern uint8_t highest_active_log_level;
extern bool log_to_serial_disabled;

extern LogHelper Logging;


#endif // GLOBALS_LOGGING_H