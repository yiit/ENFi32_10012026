#include "../Helpers/KeyValueWriter_JSON.h"

#include "../Globals/Settings.h"
#include "../Helpers/StringConverter.h"

#include "../WebServer/HTML_wrappers.h"

KeyValueWriter_JSON::KeyValueWriter_JSON(bool emptyHeader, PrintToString *toStr)
  : KeyValueWriter(emptyHeader, toStr)
{}

KeyValueWriter_JSON::KeyValueWriter_JSON(KeyValueWriter_JSON*parent, PrintToString *toStr)
  : KeyValueWriter(parent, toStr)
{}

KeyValueWriter_JSON::KeyValueWriter_JSON(bool emptyHeader, KeyValueWriter_JSON*parent, PrintToString *toStr)
  : KeyValueWriter(emptyHeader, parent, toStr)
{}

KeyValueWriter_JSON::KeyValueWriter_JSON(const String& header, PrintToString *toStr)
  : KeyValueWriter(header, nullptr, toStr)
{}

KeyValueWriter_JSON::KeyValueWriter_JSON(const __FlashStringHelper *header, PrintToString *toStr)
  : KeyValueWriter(String(header), nullptr, toStr)
{}


KeyValueWriter_JSON::KeyValueWriter_JSON(const String& header, KeyValueWriter_JSON*parent, PrintToString *toStr)
  : KeyValueWriter(header, parent, toStr)
{}

KeyValueWriter_JSON::KeyValueWriter_JSON(const __FlashStringHelper *header, KeyValueWriter_JSON*parent, PrintToString *toStr)
  : KeyValueWriter(String(header), parent, toStr)
{}

KeyValueWriter_JSON::~KeyValueWriter_JSON()
{
  if (!_isEmpty) {
    getPrint().write('\n');

    if (_hasHeader) {
#ifdef USE_KVW_JSON_INDENT
      indent();
#endif

      getPrint().write(_isArray ? ']' : '}');
    }
  }
}

void KeyValueWriter_JSON::write()
{
  if (_isEmpty) {
    if (_parent != nullptr) { _parent->write(); }

    if (_hasHeader) {
#ifdef USE_KVW_JSON_INDENT
      indent();
#endif

      if (_header.isEmpty()) {
        getPrint().write('{');
        getPrint().write('\n');
      } else {
        getPrint().print(strformat(
                           F("\"%s\":%c\n"),
                           _header.c_str(),
                           _isArray ? '[' : '{'));
      }
    }
    _isEmpty = false;
  } else {
    getPrint().write(',');
    getPrint().write('\n');
  }
}

void KeyValueWriter_JSON::write(const KeyValueStruct& kv)
{
  if (kv._format == KeyValueStruct::Format::Note) { return; }
  write();
#ifdef USE_KVW_JSON_INDENT
  indent();
  getPrint().write('\t');
#endif // ifdef USE_KVW_JSON_INDENT

  if (kv._key.length()) {
    auto& pr = getPrint();
    pr.write('"');
    pr.print(kv._key);
    pr.write('"');
    pr.write(':');
  }

  const size_t nrValues = kv._values.size();

  if (!kv._isArray) {
    // Either 1 value or empty value
    if (nrValues == 0) {
      auto& pr = getPrint();
      pr.write('"');
      pr.write('"');
    }
    else {
      writeValue(kv._values[0].get());
    }
  } else {
    // Multiple values, so we must wrap it in []
    auto& pr = getPrint();
    pr.write('[');
    pr.write('\n');

    for (size_t i = 0; i < nrValues; ++i) {
      if (i != 0) {
        auto& pr = getPrint();
        pr.write(',');
        pr.write('\n');
      }
#ifdef USE_KVW_JSON_INDENT
      indent();
      auto& pr = getPrint();
      pr.write('\t');
      pr.write('\t');
#endif // ifdef USE_KVW_JSON_INDENT

      writeValue(kv._values[i].get());
    }
    getPrint().write(']');
  }
}

void KeyValueWriter_JSON::writeValue(const ValueStruct* val)
{
  if (val == nullptr) return;
  auto& pr = getPrint();

  ValueStruct::ValueType valueType(ValueStruct::ValueType::Auto);
  String str = val->toString(valueType);

  switch (valueType)
  {
    case ValueStruct::ValueType::Float:
    case ValueStruct::ValueType::Double:
    case ValueStruct::ValueType::Int:
      pr.print(str);
      return;
    case ValueStruct::ValueType::Bool:

      if (!Settings.JSONBoolWithoutQuotes()) { pr.write('"'); }
      pr.print(str.equals("0") ? F("false") : F("true"));

      if (!Settings.JSONBoolWithoutQuotes()) { pr.write('"'); }
      return;

    case ValueStruct::ValueType::Auto:
    case ValueStruct::ValueType::String:
      break;
  }
  pr.print(to_json_value(str));
}

Sp_KeyValueWriter KeyValueWriter_JSON::createChild() { return std::make_shared<KeyValueWriter_JSON>(this, _toString); }

Sp_KeyValueWriter KeyValueWriter_JSON::createChild(const String& header)
{
  return std::make_shared<KeyValueWriter_JSON>(header, this, _toString);
}

Sp_KeyValueWriter KeyValueWriter_JSON::createNew()                     { return std::make_shared<KeyValueWriter_JSON>(false, _toString); }

Sp_KeyValueWriter KeyValueWriter_JSON::createNew(const String& header) { return std::make_shared<KeyValueWriter_JSON>(header, _toString); }


#ifdef USE_KVW_JSON_INDENT

void KeyValueWriter_JSON::indent()
{
  if (_parent != nullptr) {
    getPrint().write('\t');
    _parent->indent();
  }
}

#endif // ifdef USE_KVW_JSON_INDENT
