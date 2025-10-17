#include "../Helpers/ValueStruct_Factory.h"

Up_ValueStruct ValueStruct_Factory::create(const bool& val)
{
  std::unique_ptr<ValueStruct_T<bool> > child(new (std::nothrow) ValueStruct_T<bool>(val, ValueStruct::ValueType::Bool));

  return std::move(child);

  // return std::make_unique<ValueStruct_T<bool> >(val, ValueStruct::ValueType::Bool);
}

Up_ValueStruct ValueStruct_Factory::create(int val)
{
  std::unique_ptr<ValueStruct_T<int32_t> > child(new (std::nothrow) ValueStruct_T<int32_t>(val, ValueStruct::ValueType::Int));

  return std::move(child);

  // return std::make_unique<ValueStruct_T<int32_t> >(val, ValueStruct::ValueType::Int);
}

#if defined(ESP32) && !defined(__riscv)

Up_ValueStruct ValueStruct_Factory::create(int32_t val)
{
  std::unique_ptr<ValueStruct_T<int32_t> > child(new (std::nothrow) ValueStruct_T<int32_t>(val, ValueStruct::ValueType::Int));

  return std::move(child);

  //  return std::make_unique<ValueStruct_T<int32_t> >(val, ValueStruct::ValueType::Int);
}

#endif // if defined(ESP32) && !defined(__riscv)

Up_ValueStruct ValueStruct_Factory::create(uint32_t val)
{
  std::unique_ptr<ValueStruct_T<uint32_t> > child(new (std::nothrow) ValueStruct_T<uint32_t>(val, ValueStruct::ValueType::Int));

  return std::move(child);

  // return std::make_unique<ValueStruct_T<uint32_t> >(val, ValueStruct::ValueType::Int);
}

#if defined(ESP32) && !defined(__riscv)

Up_ValueStruct ValueStruct_Factory::create(size_t val)
{
  std::unique_ptr<ValueStruct_T<size_t> > child(new (std::nothrow) ValueStruct_T<size_t>(val, ValueStruct::ValueType::Int));

  return std::move(child);

  // return std::make_unique<ValueStruct_T<size_t> >(val, ValueStruct::ValueType::Int);
}

#endif // if defined(ESP32) && !defined(__riscv)

Up_ValueStruct ValueStruct_Factory::create(const uint64_t& val)
{
  std::unique_ptr<ValueStruct_uint64_t> child(new (std::nothrow) ValueStruct_uint64_t(val, ValueStruct::ValueType::Int));

  return std::move(child);

  //  return std::make_unique<ValueStruct_uint64_t >(val, ValueStruct::ValueType::Int);
}

Up_ValueStruct ValueStruct_Factory::create(const int64_t& val)
{

  std::unique_ptr<ValueStruct_int64_t> child(new (std::nothrow) ValueStruct_int64_t(val, ValueStruct::ValueType::Int));

  return std::move(child);

  // return std::make_unique<ValueStruct_int64_t >(val, ValueStruct::ValueType::Int);
}

Up_ValueStruct ValueStruct_Factory::create(
  const float& val,
  uint8_t                    nrDecimals,
  bool         trimTrailingZeros)
{
  std::unique_ptr<ValueStruct_Float> child(new (std::nothrow) ValueStruct_Float(val, nrDecimals, trimTrailingZeros));

  return std::move(child);

  // return std::make_unique<ValueStruct_Float>(val, nrDecimals, trimTrailingZeros);
}

Up_ValueStruct ValueStruct_Factory::create(
  const double& val,
  uint8_t                    nrDecimals,
  bool          trimTrailingZeros)
{
  std::unique_ptr<ValueStruct_Double> child(new (std::nothrow) ValueStruct_Double(val, nrDecimals, trimTrailingZeros));

  return std::move(child);

  // return std::make_unique<ValueStruct_Double>(val, nrDecimals, trimTrailingZeros);
}

Up_ValueStruct ValueStruct_Factory::create(
  const String& val)
{
  std::unique_ptr<ValueStruct_String> child(new (std::nothrow) ValueStruct_String(val));

  return std::move(child);

  // return std::make_unique<ValueStruct_String>(val);
}

Up_ValueStruct ValueStruct_Factory::create(
  String&& val)
{
  std::unique_ptr<ValueStruct_String> child(new (std::nothrow) ValueStruct_String(std::move(val)));

  return std::move(child);

  // return std::make_unique<ValueStruct_String>(std::move(val));
}

Up_ValueStruct ValueStruct_Factory::create(
  const __FlashStringHelper *val)
{
  std::unique_ptr<ValueStruct_FlashString> child(new (std::nothrow) ValueStruct_FlashString(val));

  return std::move(child);

  // return std::make_unique<ValueStruct_FlashString>(val);
}
