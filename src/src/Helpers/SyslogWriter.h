#pragma once

#include "../../ESPEasy_common.h"

#include "../Helpers/LogStreamWriter.h"


/*********************************************************************************************\
   Syslog client
\*********************************************************************************************/

class SyslogWriter : public LogStreamWriter {
public:

  SyslogWriter(uint8_t log_destination) : LogStreamWriter(log_destination) {}

  virtual bool process(Stream* stream) override;

private:

  void prepare_prefix() override;

};