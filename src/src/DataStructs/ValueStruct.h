#pragma once

#include <WString.h>
#include <Print.h>

#include <memory>

// ********************************************************************************
// ValueStruct
// ********************************************************************************
class ValueStruct
{
public:

  enum class ValueType {
    Auto,
    String,
    FlashString,
    Float,
    Double,
    Int,
    Bool

  };

  ValueStruct(ValueType valueType = ValueType::Auto) : _valueType(valueType) {}

  virtual ~ValueStruct() {}

  virtual size_t print(Print& out) const = 0;

  virtual String toString() const;

  virtual String toString(ValueType& valueType) const;

  virtual ValueType getValueType() const { return _valueType; };

protected:
  
  const ValueType _valueType;

}; // class ValueStruct

typedef std::unique_ptr<ValueStruct> Up_ValueStruct;


// ********************************************************************************
// Derived types from ValueStruct
// ********************************************************************************

class ValueStruct_String : public ValueStruct
{
public:

  virtual ~ValueStruct_String();

  ValueStruct_String(const String& val);
  ValueStruct_String(String && val);

  virtual size_t print(Print& out) const;

  String _val;
}; // class ValueStruct_String


class ValueStruct_FlashString : public ValueStruct
{
public:

  virtual ~ValueStruct_FlashString();

  ValueStruct_FlashString(const __FlashStringHelper *val);

  virtual size_t print(Print& out) const;

  const __FlashStringHelper *_val;
}; // class ValueStruct_FlashString

class ValueStruct_Double : public ValueStruct
{
public:

  virtual ~ValueStruct_Double();

  ValueStruct_Double(double val,
                     uint8_t nrDecimals = 2,
                     bool trimTrailingZeros = false);

  virtual size_t print(Print& out) const;

  virtual String toString(ValueType& valueType) const override;

  const double _val;
  const uint8_t _nrDecimals;
  const bool _trimTrailingZeros;
}; 

class ValueStruct_Float : public ValueStruct
{
public:

  virtual ~ValueStruct_Float();

  ValueStruct_Float(float val,
                     uint8_t nrDecimals = 2,
                     bool trimTrailingZeros = false);

  virtual size_t print(Print& out) const;

  virtual String toString(ValueType& valueType) const override;

  const float _val;
  const uint8_t _nrDecimals;
  const bool _trimTrailingZeros;
}; 


template<class T>
class ValueStruct_T : public ValueStruct
{
public:

  virtual ~ValueStruct_T() {}

  ValueStruct_T(T         val,
                ValueType vType)
    : ValueStruct(vType), _val(val) {}

  virtual size_t print(Print& out) const
  {
    return out.print(_val);
  }

  const T _val;
}; // class ValueStruct_T
