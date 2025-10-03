#pragma once

#include "../Helpers/KeyValueWriter.h"

class KeyValueWriter_WebForm : public KeyValueWriter
{
public:

  using KeyValueWriter::writeLabels;

  KeyValueWriter_WebForm(bool emptyHeader = false);

  KeyValueWriter_WebForm(KeyValueWriter_WebForm*parent);

  KeyValueWriter_WebForm(bool                   emptyHeader,
                         KeyValueWriter_WebForm*parent);

  KeyValueWriter_WebForm(const String& header);
  KeyValueWriter_WebForm(const __FlashStringHelper *header);

  KeyValueWriter_WebForm(const String         & header,
                         KeyValueWriter_WebForm*parent);
  KeyValueWriter_WebForm(const __FlashStringHelper *header,
                         KeyValueWriter_WebForm    *parent);


  virtual ~KeyValueWriter_WebForm();

  virtual void           write();

  virtual void           write(const KeyValueStruct& kv);

}; // class KeyValueWriter_WebForm
