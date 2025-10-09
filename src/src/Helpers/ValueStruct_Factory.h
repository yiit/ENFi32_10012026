#pragma once

#include "../DataStructs/ValueStruct.h"
#include "../Helpers/StringProvider.h"

class ValueStruct_Factory
{
public:

  static Up_ValueStruct create(const bool& val);

  static Up_ValueStruct create(int val);
#if defined(ESP32) && !defined(__riscv)
  static Up_ValueStruct create(int32_t val);
#endif
  static Up_ValueStruct create(uint32_t val);
#if defined(ESP32) && !defined(__riscv)
  static Up_ValueStruct create(size_t val);
#endif
  static Up_ValueStruct create(const uint64_t& val);

  static Up_ValueStruct create(const int64_t& val);


  static Up_ValueStruct create(const float& val,
                               int          nrDecimals        = 4,
                               bool         trimTrailingZeros = false);

  static Up_ValueStruct create(const double& val,
                               int           nrDecimals        = 4,
                               bool          trimTrailingZeros = false);

  static Up_ValueStruct create(
    LabelType::Enum label);

  static Up_ValueStruct create(
    const String         & val);

  static Up_ValueStruct create(
    String              && val);

  static Up_ValueStruct create(
    const __FlashStringHelper *val);


}; // class ValueStruct_Factory
