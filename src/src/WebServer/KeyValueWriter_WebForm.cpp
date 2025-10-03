#include "../WebServer/KeyValueWriter_WebForm.h"


#include "../WebServer/HTML_wrappers.h"
#include "../WebServer/Markup.h"

KeyValueWriter_WebForm::KeyValueWriter_WebForm(bool emptyHeader)
  : KeyValueWriter(KEYVALUEWRITER_WEBFORM, emptyHeader)
{}

KeyValueWriter_WebForm::KeyValueWriter_WebForm(KeyValueWriter_WebForm*parent)
  : KeyValueWriter(KEYVALUEWRITER_WEBFORM, parent)
{}

KeyValueWriter_WebForm::KeyValueWriter_WebForm(bool emptyHeader, KeyValueWriter_WebForm*parent)
  : KeyValueWriter(KEYVALUEWRITER_WEBFORM, emptyHeader, parent)
{}

KeyValueWriter_WebForm::KeyValueWriter_WebForm(const String& header)
  : KeyValueWriter(KEYVALUEWRITER_WEBFORM, header, nullptr)
{}

KeyValueWriter_WebForm::KeyValueWriter_WebForm(const __FlashStringHelper *header)
  : KeyValueWriter(KEYVALUEWRITER_WEBFORM, String(header), nullptr)
{}


KeyValueWriter_WebForm::KeyValueWriter_WebForm(const String& header, KeyValueWriter_WebForm*parent)
  : KeyValueWriter(KEYVALUEWRITER_WEBFORM, header, parent)
{}

KeyValueWriter_WebForm::KeyValueWriter_WebForm(const __FlashStringHelper *header, KeyValueWriter_WebForm*parent)
  : KeyValueWriter(KEYVALUEWRITER_WEBFORM, String(header), parent)
{}


KeyValueWriter_WebForm::~KeyValueWriter_WebForm()
{}

void                   KeyValueWriter_WebForm::write()
{
  if (_isEmpty) {
    if (_parent != nullptr) { _parent->write(); }

    if (_hasHeader && !_header.isEmpty()) {
      addFormSubHeader(_header);
    }
    _isEmpty = false;
  }
}

void KeyValueWriter_WebForm::write(const KeyValueStruct& kv)
{
  addRowLabel(kv._key, kv._id);
  const size_t nrValues   = kv._values.size();
  const bool   format_pre = nrValues > 1 || kv._value_pre;

  if (format_pre) { addHtml(F("<pre>")); }

  for (size_t i = 0; i < nrValues; ++i) {
    if (i != 0) {
      addHtml(F("<br>"));
    }
    addHtml(kv._values[i].str);
  }

  if (format_pre) { addHtml(F("</pre>")); }

  if (nrValues == 1) {
    addUnit(kv._unit);
  }
}
