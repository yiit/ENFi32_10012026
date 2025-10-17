#pragma once

#include "../Helpers/KeyValueWriter.h"


class KeyValueWriter_WebForm : public KeyValueWriter
{
public:

  using KeyValueWriter::writeLabels;

  KeyValueWriter_WebForm(bool           emptyHeader = false,
                         PrintToString *toStr       = nullptr);

  KeyValueWriter_WebForm(KeyValueWriter_WebForm*parent,
                         PrintToString         *toStr = nullptr);

  KeyValueWriter_WebForm(bool                   emptyHeader,
                         KeyValueWriter_WebForm*parent,
                         PrintToString         *toStr = nullptr);

  KeyValueWriter_WebForm(const String & header,
                         PrintToString *toStr = nullptr);
  KeyValueWriter_WebForm(const __FlashStringHelper *header,
                         PrintToString             *toStr = nullptr);

  KeyValueWriter_WebForm(const String         & header,
                         KeyValueWriter_WebForm*parent,
                         PrintToString         *toStr = nullptr);
  KeyValueWriter_WebForm(const __FlashStringHelper *header,
                         KeyValueWriter_WebForm    *parent,
                         PrintToString             *toStr = nullptr);


  virtual ~KeyValueWriter_WebForm();

  virtual void              write();

  virtual void              write(const KeyValueStruct& kv);

  // Create writer of the same derived type, with this set as parent
  virtual Up_KeyValueWriter createChild();
  virtual Up_KeyValueWriter createChild(const String& header);
  virtual Up_KeyValueWriter createChildArray(const String& header);

  // Create new writer of the same derived type, without parent
  virtual Up_KeyValueWriter createNew();
  virtual Up_KeyValueWriter createNew(const String& header);


}; // class KeyValueWriter_WebForm
