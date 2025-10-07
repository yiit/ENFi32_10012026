#include "../Helpers/ValueStruct_Factory.h"

Sp_ValueStruct ValueStruct_Factory::create(const bool& val)
{
  return std::make_shared<ValueStruct_T<bool> >(val, ValueStruct::ValueType::Bool);
}

Sp_ValueStruct ValueStruct_Factory::create(int val) { return std::make_shared<ValueStruct_T<int32_t> >(val, ValueStruct::ValueType::Int); }

#if defined(ESP32) && !defined(__riscv)

Sp_ValueStruct ValueStruct_Factory::create(int32_t val)
{
  return std::make_shared<ValueStruct_T<int32_t> >(val, ValueStruct::ValueType::Int);
}

#endif // if defined(ESP32) && !defined(__riscv)

Sp_ValueStruct ValueStruct_Factory::create(uint32_t val)
{
  return std::make_shared<ValueStruct_T<uint32_t> >(val, ValueStruct::ValueType::Int);
}

#if defined(ESP32) && !defined(__riscv)

Sp_ValueStruct ValueStruct_Factory::create(size_t val) { return std::make_shared<ValueStruct_T<size_t> >(val, ValueStruct::ValueType::Int); }

#endif // if defined(ESP32) && !defined(__riscv)

Sp_ValueStruct ValueStruct_Factory::create(const uint64_t& val)
{
  return std::make_shared<ValueStruct_T<uint64_t> >(val, ValueStruct::ValueType::Int);
}

Sp_ValueStruct ValueStruct_Factory::create(const int64_t& val)
{
  return std::make_shared<ValueStruct_T<int64_t> >(val, ValueStruct::ValueType::Int);
}

Sp_ValueStruct ValueStruct_Factory::create(
  const float& val,
  int          nrDecimals,
  bool         trimTrailingZeros) { return std::make_shared<ValueStruct_Float>(val, nrDecimals, trimTrailingZeros); }

Sp_ValueStruct ValueStruct_Factory::create(
  const double& val,
  int           nrDecimals,
  bool          trimTrailingZeros) { return std::make_shared<ValueStruct_Double>(val, nrDecimals, trimTrailingZeros); }

Sp_ValueStruct ValueStruct_Factory::create(
  LabelType::Enum label) { return std::make_shared<ValueStruct_String>(getValue(label), ValueStruct::ValueType::Auto); }

Sp_ValueStruct ValueStruct_Factory::create(
  const String         & val,
  ValueStruct::ValueType vType) { return std::make_shared<ValueStruct_String>(val, vType); }

Sp_ValueStruct ValueStruct_Factory::create(
  String              && val,
  ValueStruct::ValueType vType) { return std::make_shared<ValueStruct_String>(std::move(val), vType); }

Sp_ValueStruct ValueStruct_Factory::create(
  const __FlashStringHelper *val,
  ValueStruct::ValueType     vType) { return std::make_shared<ValueStruct_FlashString>(val, vType); }
