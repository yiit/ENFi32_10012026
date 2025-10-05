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

  virtual void              write();

  virtual void              write(const KeyValueStruct& kv);

  // Create writer of the same derived type, with this set as parent
  virtual Sp_KeyValueWriter createChild();
  virtual Sp_KeyValueWriter createChild(const String& header);

  // Create new writer of the same derived type, without parent
  virtual Sp_KeyValueWriter createNew();
  virtual Sp_KeyValueWriter createNew(const String& header);


}; // class KeyValueWriter_WebForm
