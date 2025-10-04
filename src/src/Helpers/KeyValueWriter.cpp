#include "../Helpers/KeyValueWriter.h"

#include "../Helpers/StringConverter_Numerical.h"

// ********************************************************************************
// ValueStruct
// ********************************************************************************

ValueStruct::ValueStruct(const String& val, ValueType vType) : str(val), valueType(vType) {}

ValueStruct::ValueStruct(const __FlashStringHelper *val, ValueType vType) : str(val), valueType(vType) {}

ValueStruct::ValueStruct(String&& val, ValueType vType) : str(std::move(val)), valueType(vType) {}


// ********************************************************************************
// KeyValueStruct
// ********************************************************************************

KeyValueStruct::KeyValueStruct(const __FlashStringHelper *key) : _key(key) {}

KeyValueStruct::KeyValueStruct(const String& key) : _key(key) {}


KeyValueStruct::KeyValueStruct(const String         & key,
                               bool                   val,
                               ValueStruct::ValueType vType)
  : _key(key) {
  _values.emplace_back(String(val), vType);
}

KeyValueStruct::KeyValueStruct(const String         & key,
                               int                    val,
                               ValueStruct::ValueType vType)
  : _key(key) {
  _values.emplace_back(String(val), vType);
}

KeyValueStruct::KeyValueStruct(const String         & key,
                               const uint64_t       & val,
                               ValueStruct::ValueType vType)
  : _key(key) {
  _values.emplace_back(ull2String(val), vType);
}

KeyValueStruct::KeyValueStruct(const String         & key,
                               const int64_t        & val,
                               ValueStruct::ValueType vType)
  : _key(key) {
  _values.emplace_back(ll2String(val), vType);
}

KeyValueStruct::KeyValueStruct(const String         & key,
                               const float          & val,
                               int                    nrDecimals,
                               ValueStruct::ValueType vType)
  : _key(key) {
  String str;

  if (!toValidString(str, val, nrDecimals)) {
    vType = ValueStruct::ValueType::String;
  }

  _values.emplace_back(str, vType);
}

KeyValueStruct::KeyValueStruct(const String         & key,
                               const double         & val,
                               int                    nrDecimals,
                               ValueStruct::ValueType vType)
  : _key(key) {
  String str;

  if (!doubleToValidString(str, val, nrDecimals)) {
    vType = ValueStruct::ValueType::String;
  }

  _values.emplace_back(str, vType);
}

KeyValueStruct::KeyValueStruct(const __FlashStringHelper *key,
                               const String             & val,
                               ValueStruct::ValueType     vType)
  : _key(key) {
  _values.emplace_back(val, vType);
}

KeyValueStruct::KeyValueStruct(const __FlashStringHelper *key,
                               const __FlashStringHelper *val,
                               ValueStruct::ValueType     vType)
  : _key(key) {
  _values.emplace_back(val, vType);
}

KeyValueStruct::KeyValueStruct(const String         & key,
                               const String         & val,
                               ValueStruct::ValueType vType)
  : _key(key) {
  _values.emplace_back(val, vType);
}

KeyValueStruct::KeyValueStruct(const __FlashStringHelper *key,
                               String                  && val,
                               ValueStruct::ValueType     vType)
  : _key(key) {
  _values.emplace_back(std::move(val), vType);
}

KeyValueStruct::KeyValueStruct(const String         & key,
                               String              && val,
                               ValueStruct::ValueType vType)
  : _key(key) {
  _values.emplace_back(std::move(val), vType);
}

KeyValueStruct::KeyValueStruct(LabelType::Enum label)
  : _key(getLabel(label)), _unit(getFormUnit(label)) {
  _values.emplace_back(getValue(label));
}

void KeyValueStruct::setUnit(const String& unit)              { _unit = unit; }

void KeyValueStruct::setUnit(const __FlashStringHelper *unit) { _unit = unit; }

void KeyValueStruct::appendValue(const ValueStruct& value)
{
  _values.emplace_back(std::move(value));
  _isArray = true;
}

void KeyValueStruct::appendValue(ValueStruct&& value)
{
  _values.emplace_back(std::move(value));
  _isArray = true;
}

// ********************************************************************************
// KeyValueWriter
// ********************************************************************************

void KeyValueWriter::clear() {
  _isEmpty = true;

  if (_toString) {
    _toString->clear();
  }
}

void KeyValueWriter::writeLabels(const LabelType::Enum labels[])
{
  size_t i            = 0;
  LabelType::Enum cur = static_cast<const LabelType::Enum>(pgm_read_byte(labels + i));

  while (cur != LabelType::MAX_LABEL) {
    write(cur);
    cur = static_cast<const LabelType::Enum>(pgm_read_byte(labels + i + 1));
    ++i;
  }
}

int KeyValueWriter::getLevel() const
{
  if (_parent == nullptr) { return 0; }
  return _parent->getLevel() + 1;
}
