#include "../DataStructs/ValueStruct.h"

#include "../Helpers/PrintToString.h"
#include "../Helpers/StringConverter_Numerical.h"

// ********************************************************************************
// ValueStruct
// ********************************************************************************
String ValueStruct::toString() const
{
  PrintToString p;

  print(p);
  String res(p.getMove());
  return res;
}

String ValueStruct::toString(ValueType& valueType) const
{
  valueType = getValueType();
  return toString();
}

// ********************************************************************************
// ValueStruct_String
// ********************************************************************************

ValueStruct_String::~ValueStruct_String() {}

ValueStruct_String::ValueStruct_String(const String& val)
  : ValueStruct(ValueType::String), _val(val) {}

ValueStruct_String::ValueStruct_String(String&& val)
  : ValueStruct(ValueType::String), _val(std::move(val)) {}

size_t ValueStruct_String::print(Print& out) const
{
  return out.print(_val);
}

// ********************************************************************************
// ValueStruct_FlashString
// ********************************************************************************

ValueStruct_FlashString::~ValueStruct_FlashString() {}

ValueStruct_FlashString::ValueStruct_FlashString(const __FlashStringHelper *val)
  : ValueStruct(ValueType::FlashString), _val(val) {}

size_t ValueStruct_FlashString::print(Print& out) const
{
  return out.print(_val);
}

// ********************************************************************************
// ValueStruct_Double
// ********************************************************************************

ValueStruct_Double::~ValueStruct_Double() {}

ValueStruct_Double::ValueStruct_Double(
  double    val,
  uint8_t  nrDecimals,
  bool      trimTrailingZeros)
  : ValueStruct(ValueType::Double), _val(val), _nrDecimals(nrDecimals), _trimTrailingZeros(trimTrailingZeros) {}

size_t ValueStruct_Double::print(Print& out) const
{
  return out.print(doubleToString(_val, _nrDecimals, _trimTrailingZeros));
}

String ValueStruct_Double::toString(ValueType& valueType) const
{
  String res;

  valueType = getValueType();

  if (!doubleToValidString(res, _val, _nrDecimals, _trimTrailingZeros)) {
    valueType = ValueType::String;
  }

  return res;
}

// ********************************************************************************
// ValueStruct_Float
// ********************************************************************************

ValueStruct_Float::~ValueStruct_Float() {}

ValueStruct_Float::ValueStruct_Float(
  float     val,
  uint8_t  nrDecimals,
  bool      trimTrailingZeros)
  : ValueStruct(ValueType::Float), _val(val), _nrDecimals(nrDecimals), _trimTrailingZeros(trimTrailingZeros) {}

size_t ValueStruct_Float::print(Print& out) const
{
  String res;

  toValidString(res, _val, _nrDecimals, _trimTrailingZeros);
  return out.print(res);
}

String ValueStruct_Float::toString(ValueType& valueType) const
{
  String res;

  valueType = getValueType();

  if (!toValidString(res, _val, _nrDecimals, _trimTrailingZeros)) {
    valueType = ValueType::String;
  }

  return res;
}
