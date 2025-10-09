#include "../DataStructs/KeyValueStruct.h"

#include "../Helpers/StringConverter_Numerical.h"


// ********************************************************************************
// KeyValueStruct
// ********************************************************************************

KeyValueStruct::KeyValueStruct(const __FlashStringHelper *key, Format format) : _key(key), _format(format) {}

KeyValueStruct::KeyValueStruct(const String& key, Format format) : _key(key), _format(format) {}


KeyValueStruct::KeyValueStruct(const String& key,
                               const bool  & val,
                               Format        format)
  : _key(key), _format(format) {
  _values.emplace_back(ValueStruct_Factory::create(val));
}

KeyValueStruct::KeyValueStruct(const String& key,
                               int           val,
                               Format        format)
  : _key(key), _format(format) {
  _values.emplace_back(ValueStruct_Factory::create(val));
}

#if defined(ESP32) && !defined(__riscv)
KeyValueStruct::KeyValueStruct(const String& key,
                               int32_t       val,
                               Format        format)
  : _key(key), _format(format) {
  _values.emplace_back(ValueStruct_Factory::create(val));
}

#endif // if defined(ESP32) && !defined(__riscv)
KeyValueStruct::KeyValueStruct(const String& key,
                               uint32_t      val,
                               Format        format)
  : _key(key), _format(format) {
  _values.emplace_back(ValueStruct_Factory::create(val));
}

#if defined(ESP32) && !defined(__riscv)
KeyValueStruct::KeyValueStruct(const String& key,
                               size_t        val,
                               Format        format)
  : _key(key), _format(format) {
  _values.emplace_back(ValueStruct_Factory::create(val));
}

#endif // if defined(ESP32) && !defined(__riscv)
KeyValueStruct::KeyValueStruct(const String  & key,
                               const uint64_t& val,
                               Format          format)
  : _key(key), _format(format) {
  _values.emplace_back(ValueStruct_Factory::create(val));
}

KeyValueStruct::KeyValueStruct(const String & key,
                               const int64_t& val,
                               Format         format)
  : _key(key), _format(format) {
  _values.emplace_back(ValueStruct_Factory::create(val));
}

KeyValueStruct::KeyValueStruct(const String& key,
                               const float & val,
                               int           nrDecimals,
                               Format        format)
  : _key(key), _format(format) {
  _values.emplace_back(ValueStruct_Factory::create(val, nrDecimals));
}

KeyValueStruct::KeyValueStruct(const String& key,
                               const double& val,
                               int           nrDecimals,
                               Format        format)
  : _key(key), _format(format) {
  _values.emplace_back(ValueStruct_Factory::create(val, nrDecimals));
}

KeyValueStruct::KeyValueStruct(const __FlashStringHelper *key,
                               const String             & val,
                               Format                     format)
  : _key(key), _format(format) {
  _values.emplace_back(ValueStruct_Factory::create(val));
}

KeyValueStruct::KeyValueStruct(const String             & key,
                               const __FlashStringHelper *val,
                               Format                     format)
  : _key(key), _format(format) {
  _values.emplace_back(ValueStruct_Factory::create(val));
}

KeyValueStruct::KeyValueStruct(const __FlashStringHelper *key,
                               const __FlashStringHelper *val,
                               Format                     format)
  : _key(key), _format(format) {
  _values.emplace_back(ValueStruct_Factory::create(val));
}

KeyValueStruct::KeyValueStruct(const __FlashStringHelper *key,
                               const char                *val,
                               Format                     format)
  : _key(key), _format(format) {
  _values.emplace_back(ValueStruct_Factory::create(String(val)));
}

KeyValueStruct::KeyValueStruct(const String         & key,
                               const String         & val,
                               Format                 format)
  : _key(key), _format(format) {
  _values.emplace_back(ValueStruct_Factory::create(val));
}

KeyValueStruct::KeyValueStruct(const __FlashStringHelper *key,
                               String                  && val,
                               Format                     format)
  : _key(key), _format(format) {
  _values.emplace_back(ValueStruct_Factory::create(std::move(val)));
}

KeyValueStruct::KeyValueStruct(const String         & key,
                               String              && val,
                               Format                 format)
  : _key(key), _format(format) {
  _values.emplace_back(ValueStruct_Factory::create(std::move(val)));
}

KeyValueStruct::KeyValueStruct(LabelType::Enum label, Format format)
  : _key(getLabel(label)), _unit(getFormUnit(label)), _format(format) {
  _values.emplace_back(ValueStruct_Factory::create(label));
}

void KeyValueStruct::setUnit(const String& unit)              { _unit = unit; }

void KeyValueStruct::setUnit(const __FlashStringHelper *unit) { _unit = unit; }

void KeyValueStruct::appendValue(Up_ValueStruct value)
{
  _values.emplace_back(std::move(value));
  _isArray = true;
}

void KeyValueStruct::appendValue(const String& value)
{
  _values.emplace_back(ValueStruct_Factory::create(value));
  _isArray = true;
}

void KeyValueStruct::appendValue(String&& value)
{
  _values.emplace_back(ValueStruct_Factory::create(std::move(value)));
  _isArray = true;
}
