#include "../DataTypes/ESPEasy_key_value_store_data.h"

constexpr unsigned int size_64bit = sizeof(double);

ESPEasy_key_value_store_8byte_data_t::ESPEasy_key_value_store_8byte_data_t() {
  ZERO_FILL(binary);
}

ESPEasy_key_value_store_8byte_data_t::ESPEasy_key_value_store_8byte_data_t(const ESPEasy_key_value_store_8byte_data_t& other)
{
  memcpy(binary, other.binary, sizeof(binary));
}

ESPEasy_key_value_store_8byte_data_t& ESPEasy_key_value_store_8byte_data_t::operator=(const ESPEasy_key_value_store_8byte_data_t& other)
{
  memcpy(binary, other.binary, sizeof(binary));
  return *this;
}

void    ESPEasy_key_value_store_8byte_data_t::clear() { ZERO_FILL(binary); }

int64_t ESPEasy_key_value_store_8byte_data_t::getInt64() const
{
  int64_t res{};

  memcpy(&res, binary, size_64bit);
  return res;
}

void     ESPEasy_key_value_store_8byte_data_t::setInt64(int64_t value) { memcpy(binary, &value, size_64bit); }

uint64_t ESPEasy_key_value_store_8byte_data_t::getUint64() const
{
  uint64_t res{};

  memcpy(&res, binary, size_64bit);
  return res;
}

void   ESPEasy_key_value_store_8byte_data_t::setUint64(uint64_t value) { memcpy(binary, &value, size_64bit); }

double ESPEasy_key_value_store_8byte_data_t::getDouble() const
{
  double res{};

  memcpy(&res, binary, size_64bit);
  return res;
}

void ESPEasy_key_value_store_8byte_data_t::setDouble(double value) { memcpy(binary, &value, size_64bit); }
