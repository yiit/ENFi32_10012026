#include "../WebServer/KeyValueWriter_WebForm.h"

#include "../Helpers/StringConverter.h"

#include "../WebServer/HTML_wrappers.h"
#include "../WebServer/Markup.h"


KeyValueWriter_WebForm::KeyValueWriter_WebForm(bool emptyHeader, PrintToString *toStr)
  : KeyValueWriter(emptyHeader, toStr)
{}

KeyValueWriter_WebForm::KeyValueWriter_WebForm(KeyValueWriter_WebForm*parent, PrintToString *toStr)
  : KeyValueWriter(parent, toStr)
{}

KeyValueWriter_WebForm::KeyValueWriter_WebForm(bool emptyHeader, KeyValueWriter_WebForm*parent, PrintToString *toStr)
  : KeyValueWriter(emptyHeader, parent, toStr)
{}

KeyValueWriter_WebForm::KeyValueWriter_WebForm(const String& header, PrintToString *toStr)
  : KeyValueWriter(header, nullptr, toStr)
{}

KeyValueWriter_WebForm::KeyValueWriter_WebForm(const __FlashStringHelper *header, PrintToString *toStr)
  : KeyValueWriter(String(header), nullptr, toStr)
{}


KeyValueWriter_WebForm::KeyValueWriter_WebForm(const String& header, KeyValueWriter_WebForm*parent, PrintToString *toStr)
  : KeyValueWriter(header, parent, toStr)
{}

KeyValueWriter_WebForm::KeyValueWriter_WebForm(const __FlashStringHelper *header, KeyValueWriter_WebForm*parent, PrintToString *toStr)
  : KeyValueWriter(String(header), parent, toStr)
{}

KeyValueWriter_WebForm::~KeyValueWriter_WebForm()
{
  if (!summaryValueOnly() && !_isEmpty && _hasHeader && _header.isEmpty()) {
    // TODO TD-er: Should we add a separator line here?
  }

}

void KeyValueWriter_WebForm::write()
{
  if (_isEmpty) {
    if (_parent != nullptr) { _parent->write(); }

    if (!summaryValueOnly() && _hasHeader && !_header.isEmpty()) {
      if (plainText()) {
        getPrint().print(concat(_header, F(": ")));
      } else {
        addFormSubHeader(_header);
      }
    }
    _isEmpty = false;
  }
}

void KeyValueWriter_WebForm::write(const KeyValueStruct& kv)
{
  write();
  const bool plain_text         = plainText();
  const bool summary_value_only = summaryValueOnly();

  const size_t nrValues   = kv._values.size();
  const bool   format_pre =
    !plain_text &&
    !summary_value_only &&
    (nrValues > 1 || kv._format == KeyValueStruct::Format::PreFormatted);

  const bool format_note =
    !plain_text &&
    !summary_value_only &&
    (nrValues == 1 && kv._format == KeyValueStruct::Format::Note);

  if (!summary_value_only) {
    if (format_note) { addRowLabel_tr_id(EMPTY_STRING, EMPTY_STRING); }
    else {
      addRowLabel(kv._key, kv._id);
    }
  }

  if (format_note) { getPrint().print(F(" <div class='note'>Note: ")); }

  if (format_pre) { getPrint().print(F("<pre>")); }

  for (size_t i = 0; i < nrValues; ++i) {
    if (i != 0) {
      if (plain_text) {
        getPrint().write('\n');
      }
      else {
        getPrint().print(F("<br>"));
      }
    }

    if (!plain_text && (kv._values[i].str.indexOf('\n') != -1))
    {
      String str = kv._values[i].str;
      str.replace(F("\n"), F("<br>"));
      getPrint().print(str);
    } else {
      getPrint().print(kv._values[i].str);
    }
  }

  // May need to include the unit before ending </pre>
  // or else it will be shown on the next line
  if (!plain_text && (nrValues == 1) && !summary_value_only) {
    addUnit(kv._unit);
  }

  if (format_pre) { getPrint().print(F("</pre>")); }

  if (format_note) { getPrint().print(F("</div>")); }

  if (summary_value_only) {
    // Must add a newline
    if (plain_text) {
      getPrint().write('\n');
    }
    else {
      getPrint().print(F("<br>"));
    }
  }
}

Sp_KeyValueWriter KeyValueWriter_WebForm::createChild()                     { return std::make_shared<KeyValueWriter_WebForm>(this); }

Sp_KeyValueWriter KeyValueWriter_WebForm::createChild(const String& header) { return std::make_shared<KeyValueWriter_WebForm>(header, this); }

Sp_KeyValueWriter KeyValueWriter_WebForm::createNew()                       { return std::make_shared<KeyValueWriter_WebForm>(); }

Sp_KeyValueWriter KeyValueWriter_WebForm::createNew(const String& header)   { return std::make_shared<KeyValueWriter_WebForm>(header); }
