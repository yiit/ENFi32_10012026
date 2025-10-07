#pragma once

#include "../DataStructs/ValueStruct.h"
#include "../Helpers/StringProvider.h"

class ValueStruct_Factory
{
public:

  static Sp_ValueStruct create(const bool& val);

  static Sp_ValueStruct create(int val);
#if defined(ESP32) && !defined(__riscv)
  static Sp_ValueStruct create(int32_t val);
#endif
  static Sp_ValueStruct create(uint32_t val);
#if defined(ESP32) && !defined(__riscv)
  static Sp_ValueStruct create(size_t val);
#endif
  static Sp_ValueStruct create(const uint64_t& val);

  static Sp_ValueStruct create(const int64_t& val);


  static Sp_ValueStruct create(const float& val,
                               int          nrDecimals        = 4,
                               bool         trimTrailingZeros = false);

  static Sp_ValueStruct create(const double& val,
                               int           nrDecimals        = 4,
                               bool          trimTrailingZeros = false);

  static Sp_ValueStruct create(
    LabelType::Enum label);

  static Sp_ValueStruct create(
    const String         & val,
    ValueStruct::ValueType vType = ValueStruct::ValueType::String);

  static Sp_ValueStruct create(
    String              && val,
    ValueStruct::ValueType vType = ValueStruct::ValueType::String);

  static Sp_ValueStruct create(
    const __FlashStringHelper *val,
    ValueStruct::ValueType     vType = ValueStruct::ValueType::String);


}; // class ValueStruct_Factory
