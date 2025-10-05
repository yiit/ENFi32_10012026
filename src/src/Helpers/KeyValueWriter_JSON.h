#pragma once

#include "../Helpers/KeyValueWriter.h"

#ifndef BUILD_NO_DEBUG
 # define USE_KVW_JSON_INDENT
#endif

class KeyValueWriter_JSON : public KeyValueWriter
{
public:

  /*
     TODO TD-er: Replace
     - stream_to_json_object_value used in ChartJS code

   */

  using KeyValueWriter::writeLabels;

  KeyValueWriter_JSON(bool           emptyHeader = false,
                      PrintToString *toStr       = nullptr);

  KeyValueWriter_JSON(KeyValueWriter_JSON*parent,
                      PrintToString      *toStr = nullptr);

  KeyValueWriter_JSON(bool                emptyHeader,
                      KeyValueWriter_JSON*parent,
                      PrintToString      *toStr = nullptr);

  KeyValueWriter_JSON(const String & header,
                      PrintToString *toStr = nullptr);
  KeyValueWriter_JSON(const __FlashStringHelper *header,
                      PrintToString             *toStr = nullptr);

  KeyValueWriter_JSON(const String      & header,
                      KeyValueWriter_JSON*parent,
                      PrintToString      *toStr = nullptr);
  KeyValueWriter_JSON(const __FlashStringHelper *header,
                      KeyValueWriter_JSON       *parent,
                      PrintToString             *toStr = nullptr);

  virtual ~KeyValueWriter_JSON();

  virtual void              write();

  virtual void              write(const KeyValueStruct& kv);

  // Create writer of the same derived type, with this set as parent
  virtual Sp_KeyValueWriter createChild();
  virtual Sp_KeyValueWriter createChild(const String& header);

  // Create new writer of the same derived type, without parent
  virtual Sp_KeyValueWriter createNew();
  virtual Sp_KeyValueWriter createNew(const String& header);

private:

  void writeValue(const ValueStruct& value);

#ifdef USE_KVW_JSON_INDENT

public:

  virtual void indent() override;
#endif // ifdef USE_KVW_JSON_INDENT


}; // class KeyValueWriter_JSON
