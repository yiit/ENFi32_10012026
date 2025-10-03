#pragma once

#include "../Helpers/KeyValueWriter.h"

#ifndef BUILD_NO_DEBUG
 # define USE_KVW_JSON_INDENT
#endif

class KeyValueWriter_JSON : public KeyValueWriter
{
public:

  using KeyValueWriter::writeLabels;

  KeyValueWriter_JSON(bool emptyHeader = false);

  KeyValueWriter_JSON(KeyValueWriter_JSON*parent);

  KeyValueWriter_JSON(bool                emptyHeader,
                      KeyValueWriter_JSON*parent);

  KeyValueWriter_JSON(const String& header);
  KeyValueWriter_JSON(const __FlashStringHelper *header);

  KeyValueWriter_JSON(const String      & header,
                      KeyValueWriter_JSON*parent);
  KeyValueWriter_JSON(const __FlashStringHelper *header,
                      KeyValueWriter_JSON       *parent);

  virtual ~KeyValueWriter_JSON();

  virtual void        clear() override;

  virtual void        write();

  virtual void        write(const KeyValueStruct& kv);

private:

  void writeValue(const ValueStruct& value);

#ifdef USE_KVW_JSON_INDENT

public:

  virtual void indent() const override;
#endif // ifdef USE_KVW_JSON_INDENT


}; // class KeyValueWriter_JSON
