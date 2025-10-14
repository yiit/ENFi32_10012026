#pragma once

#include "../../ESPEasy_common.h"

#define LOG_LEVEL_NONE                      0  // console output is also loglevel none, only shown on those 'log destinations' subscribing to get console output.
#define LOG_LEVEL_ERROR                     1
#define LOG_LEVEL_INFO                      2
#ifndef BUILD_NO_DEBUG
# define LOG_LEVEL_DEBUG                     3
# define LOG_LEVEL_DEBUG_MORE                4
# define LOG_LEVEL_DEBUG_DEV                 9 // use for testing/debugging only, not for regular use
# define LOG_LEVEL_NRELEMENTS                5 // Update this and getLogLevelDisplayString() when new log levels are added
#else // ifndef BUILD_NO_DEBUG
# define LOG_LEVEL_NRELEMENTS                2 // Update this and getLogLevelDisplayString() when new log levels are added
#endif // ifndef BUILD_NO_DEBUG

#define LOG_TO_SERIAL         0
#define LOG_TO_SYSLOG         1
#define LOG_TO_WEBLOG         2
#define LOG_TO_SDCARD         3
#define LOG_TO_SERIAL_EXTRA   4

#define NR_LOG_TO_DESTINATIONS  5   // Update this when adding extra log output streams


#ifdef BUILD_NO_DEBUG
#define LOG_LEVEL_MAX_STRING_LENGTH  6
#else
#define LOG_LEVEL_MAX_STRING_LENGTH  10
#endif