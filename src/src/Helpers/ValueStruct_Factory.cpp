#include "../Helpers/ValueStruct_Factory.h"

Up_ValueStruct ValueStruct_Factory::create(const bool& val)
{
  return std::make_unique<ValueStruct_T<bool> >(val, ValueStruct::ValueType::Bool);
}

Up_ValueStruct ValueStruct_Factory::create(int val) { return std::make_unique<ValueStruct_T<int32_t> >(val, ValueStruct::ValueType::Int); }

#if defined(ESP32) && !defined(__riscv)

Up_ValueStruct ValueStruct_Factory::create(int32_t val)
{
  return std::make_unique<ValueStruct_T<int32_t> >(val, ValueStruct::ValueType::Int);
}

#endif // if defined(ESP32) && !defined(__riscv)

Up_ValueStruct ValueStruct_Factory::create(uint32_t val)
{
  return std::make_unique<ValueStruct_T<uint32_t> >(val, ValueStruct::ValueType::Int);
}

#if defined(ESP32) && !defined(__riscv)

Up_ValueStruct ValueStruct_Factory::create(size_t val) { return std::make_unique<ValueStruct_T<size_t> >(val, ValueStruct::ValueType::Int); }

#endif // if defined(ESP32) && !defined(__riscv)

Up_ValueStruct ValueStruct_Factory::create(const uint64_t& val)
{
  return std::make_unique<ValueStruct_T<uint64_t> >(val, ValueStruct::ValueType::Int);
}

Up_ValueStruct ValueStruct_Factory::create(const int64_t& val)
{
  return std::make_unique<ValueStruct_T<int64_t> >(val, ValueStruct::ValueType::Int);
}

Up_ValueStruct ValueStruct_Factory::create(
  const float& val,
  int          nrDecimals,
  bool         trimTrailingZeros) { return std::make_unique<ValueStruct_Float>(val, nrDecimals, trimTrailingZeros); }

Up_ValueStruct ValueStruct_Factory::create(
  const double& val,
  int           nrDecimals,
  bool          trimTrailingZeros) { return std::make_unique<ValueStruct_Double>(val, nrDecimals, trimTrailingZeros); }

Up_ValueStruct ValueStruct_Factory::create(
  LabelType::Enum label) { return std::make_unique<ValueStruct_String>(getValue(label)); }

Up_ValueStruct ValueStruct_Factory::create(
  const String         & val) { return std::make_unique<ValueStruct_String>(val); }

Up_ValueStruct ValueStruct_Factory::create(
  String              && val) { return std::make_unique<ValueStruct_String>(std::move(val)); }

Up_ValueStruct ValueStruct_Factory::create(
  const __FlashStringHelper *val) { return std::make_unique<ValueStruct_FlashString>(val); }
