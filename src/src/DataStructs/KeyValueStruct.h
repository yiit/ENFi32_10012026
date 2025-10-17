#pragma once

#include "../DataStructs/ValueStruct.h"

#include "../Helpers/StringProvider.h"
#include "../Helpers/ValueStruct_Factory.h"

#include <vector>

// ********************************************************************************
// KeyValueStruct
// ********************************************************************************
struct KeyValueStruct
{
  enum class Format {
    Default,
    PreFormatted,
    Note

  };


  KeyValueStruct(Format format = Format::Default) : _format(format) {}

  KeyValueStruct(const __FlashStringHelper *key,
                 Format                     format = Format::Default);
  KeyValueStruct(const String& key,
                 Format        format = Format::Default);

  KeyValueStruct(const String         & key,
                 const bool           & val,
                 Format                 format = Format::Default);

  KeyValueStruct(const String         & key,
                 int                    val,
                 Format                 format = Format::Default);
#if defined(ESP32) && !defined(__riscv)
  KeyValueStruct(const String         & key,
                 int32_t                val,
                 Format                 format = Format::Default);
#endif
  KeyValueStruct(const String         & key,
                 uint32_t               val,
                 Format                 format = Format::Default);
#if defined(ESP32) && !defined(__riscv)
  KeyValueStruct(const String         & key,
                 size_t                 val,
                 Format                 format = Format::Default);
#endif
  KeyValueStruct(const String         & key,
                 const uint64_t       & val,
                 Format                 format = Format::Default);

  KeyValueStruct(const String         & key,
                 const int64_t        & val,
                 Format                 format = Format::Default);


  KeyValueStruct(const String         & key,
                 const float          & val,
                 uint8_t                    nrDecimals = 4,
                 Format                 format     = Format::Default);

  KeyValueStruct(const String         & key,
                 const double         & val,
                 uint8_t                    nrDecimals = 4,
                 Format                 format     = Format::Default);


  KeyValueStruct(const __FlashStringHelper *key,
                 const String             & val,
                 Format                     format = Format::Default);

  KeyValueStruct(const String             & key,
                 const __FlashStringHelper *val,
                 Format                     format = Format::Default);

  KeyValueStruct(const __FlashStringHelper *key,
                 const __FlashStringHelper *val,
                 Format                     format = Format::Default);

  KeyValueStruct(const __FlashStringHelper *key,
                 const char                *val,
                 Format                     format = Format::Default);

  KeyValueStruct(const String         & key,
                 const String         & val,
                 Format                 format = Format::Default);

  KeyValueStruct(const __FlashStringHelper *key,
                 String                  && val,
                 Format                     format = Format::Default);


  KeyValueStruct(const String         & key,
                 String              && val,
                 Format                 format = Format::Default);

  /*
     // TD-er: Do not use template types as it may 'explode' in binary size.
     // For example "foo" and "test" will result in 2 compiled instances
     // as they are  const char[3] and const char[4] respectively.
     template<typename T>
     KeyValueStruct(const String         & key,
                    const T              & val,
                    Format                 format = Format::Default,
                    ValueStruct::ValueType vType  = ValueStruct::ValueType::Auto)
      : _key(key), _format(format) {
      _values.emplace_back(String(val), vType);
     }
   */

  void setUnit(const String& unit);
  void setUnit(const __FlashStringHelper *unit);

  void setID(const String& id);
  void setID(const __FlashStringHelper *id);

  void appendValue(Up_ValueStruct value);
  void appendValue(const String& value);
  void appendValue(String&& value);

  String getID() const;


  String _key;
  String __id;
  String _unit;

  std::vector<Up_ValueStruct>_values;

  Format _format = Format::Default;

  bool _isArray{};

};