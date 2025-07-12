#include "../Helpers/_ESPEasy_key_value_store.h"

#include "../Helpers/StringConverter_Numerical.h"

uint32_t combine_StorageType_and_key(
  ESPEasy_key_value_store::StorageType storageType,
  uint32_t                             key)
{
  if (storageType == ESPEasy_key_value_store::StorageType::bool_false) {
    // bool type will be kept in memory as bool_type
    storageType = ESPEasy_key_value_store::StorageType::bool_type;
  }

  uint32_t res = key & ((1 << 24) - 1);
  res |= (static_cast<uint32_t>(storageType) << 24);
  return res;
}

ESPEasy_key_value_store::StorageType get_StorageType_from_key(uint32_t key)
{
  return static_cast<ESPEasy_key_value_store::StorageType>(key >> 24);
}

size_t getStorageSizePerType(ESPEasy_key_value_store::StorageType storageType)
{
  switch (storageType)
  {
    case ESPEasy_key_value_store::StorageType::not_set:      break;
    case ESPEasy_key_value_store::StorageType::string_type:  break;
    case ESPEasy_key_value_store::StorageType::int8_type:    return 4 + 1;
    case ESPEasy_key_value_store::StorageType::uint8_type:   return 4 + 1;
    case ESPEasy_key_value_store::StorageType::int16_type:   return 4 + 2;
    case ESPEasy_key_value_store::StorageType::uint16_type:  return 4 + 2;
    case ESPEasy_key_value_store::StorageType::int32_type:   return 4 + 4;
    case ESPEasy_key_value_store::StorageType::uint32_type:  return 4 + 4;
    case ESPEasy_key_value_store::StorageType::int64_type:   return 4 + 8;
    case ESPEasy_key_value_store::StorageType::uint64_type:  return 4 + 8;
    case ESPEasy_key_value_store::StorageType::float_type:   return 4 + 4;
    case ESPEasy_key_value_store::StorageType::double_type:  return 4 + 8;
    case ESPEasy_key_value_store::StorageType::bool_type:    return 4 + 0;
    case ESPEasy_key_value_store::StorageType::MAX_Type:     break;
    case ESPEasy_key_value_store::StorageType::bool_true:
    case ESPEasy_key_value_store::StorageType::bool_false:   return 4 + 0;
  }
  return 0;
}

// TODO TD-er: Implement
bool   ESPEasy_key_value_store::load()  { return false; }

// TODO TD-er: Implement
bool   ESPEasy_key_value_store::store() { return false; }

size_t ESPEasy_key_value_store::getStorageSize() const
{
  size_t res{};

  for (auto it = _string_data.begin(); it != _string_data.end(); ++it)
  {
    res += 4; // key size
    res += it->second.length();
  }

  for (auto it = _4byte_data.begin(); it != _4byte_data.end(); ++it)
  {
    res += getStorageSizePerType(
      static_cast<ESPEasy_key_value_store::StorageType>(it->first));
  }

  for (auto it = _8byte_data.begin(); it != _8byte_data.end(); ++it)
  {
    res += getStorageSizePerType(
      static_cast<ESPEasy_key_value_store::StorageType>(it->first));
  }
  return res;
}

bool ESPEasy_key_value_store::hasKey(StorageType storageType, uint32_t key) const
{
  if (!hasStorageType(storageType)) { return false; }

  if (storageType == ESPEasy_key_value_store::StorageType::string_type)
  {
    return _string_data.find(key) != _string_data.end();
  }
  const uint32_t combined_key = combine_StorageType_and_key(storageType, key);
  const size_t   size         = getStorageSizePerType(storageType);

  if (size > 8)
  {
    return _8byte_data.find(key) != _8byte_data.end();
  }

  if (size >= 4) {
    return _4byte_data.find(key) != _4byte_data.end();
  }
  return false;
}

ESPEasy_key_value_store::StorageType ESPEasy_key_value_store::getStorageType(uint32_t key) const
{
  uint32_t cached_bitmap = _storage_type_present_cache;

  // Search all storage types, skipping the 'not_set'
  for (size_t i = 0;
       cached_bitmap &&
       i < static_cast<size_t>(ESPEasy_key_value_store::StorageType::MAX_Type);
       ++i)
  {
    const ESPEasy_key_value_store::StorageType res = static_cast<ESPEasy_key_value_store::StorageType>(i);

    if (hasKey(res, key)) { return res; }
    cached_bitmap >>= 1;
  }
  return ESPEasy_key_value_store::StorageType::not_set;
}

bool ESPEasy_key_value_store::getValue(uint32_t key, String& value) const
{
  auto it = _string_data.find(key);

  if (it == _string_data.end()) { return false; }
  value = it->second;
  return true;
}

void ESPEasy_key_value_store::setValue(uint32_t key, const String& value)
{
  _string_data[key] = value;
  setHasStorageType(ESPEasy_key_value_store::StorageType::string_type);
}

#define GET_4BYTE_TYPE(T, GF)                                        \
        if (hasStorageType(ESPEasy_key_value_store::StorageType::T)) \
        { const uint32_t combined_key = combine_StorageType_and_key( \
            ESPEasy_key_value_store::StorageType::T, key);           \
          auto it = _4byte_data.find(combined_key);                  \
          if (it != _4byte_data.end()) {                             \
            value = it->second.GF();                                 \
            return true;                                             \
          }                                                          \
        }                                                            \
        return false;

#define GET_8BYTE_TYPE(T, GF)                                        \
        if (hasStorageType(ESPEasy_key_value_store::StorageType::T)) \
        { const uint32_t combined_key = combine_StorageType_and_key( \
            ESPEasy_key_value_store::StorageType::T, key);           \
          auto it = _8byte_data.find(combined_key);                  \
          if (it != _8byte_data.end()) {                             \
            value = it->second.GF();                                 \
            return true;                                             \
          }                                                          \
        }                                                            \
        return false;


#define SET_4BYTE_TYPE(T, SF)                                      \
        const uint32_t combined_key = combine_StorageType_and_key( \
          ESPEasy_key_value_store::StorageType::T, key);           \
        _4byte_data[combined_key].SF(value);                       \
        setHasStorageType(ESPEasy_key_value_store::StorageType::T);


#define SET_8BYTE_TYPE(T, SF)                                      \
        const uint32_t combined_key = combine_StorageType_and_key( \
          ESPEasy_key_value_store::StorageType::T, key);           \
        _8byte_data[combined_key].SF(value);                       \
        setHasStorageType(ESPEasy_key_value_store::StorageType::T);

bool ESPEasy_key_value_store::getValue(uint32_t key, int8_t& value) const
{
  GET_4BYTE_TYPE(int8_type, getInt32)
}

void ESPEasy_key_value_store::setValue(uint32_t key, const int8_t& value) { SET_4BYTE_TYPE(int8_type, setInt32) }

bool ESPEasy_key_value_store::getValue(uint32_t key, uint8_t& value) const
{
  GET_4BYTE_TYPE(uint8_type, getUint32);
}

void ESPEasy_key_value_store::setValue(uint32_t key, const uint8_t& value) { SET_4BYTE_TYPE(uint8_type, setUint32); }

bool ESPEasy_key_value_store::getValue(uint32_t key, int16_t& value) const
{
  GET_4BYTE_TYPE(int16_type, getInt32);
}

void ESPEasy_key_value_store::setValue(uint32_t key, const int16_t& value) { SET_4BYTE_TYPE(int16_type, setInt32); }

bool ESPEasy_key_value_store::getValue(uint32_t key, uint16_t& value) const
{
  GET_4BYTE_TYPE(uint16_type, getUint32);

}

void ESPEasy_key_value_store::setValue(uint32_t key, const uint16_t& value) { SET_4BYTE_TYPE(uint16_type, setUint32); }

bool ESPEasy_key_value_store::getValue(uint32_t key, int32_t& value) const
{
  GET_4BYTE_TYPE(int32_type, getInt32);
}

void ESPEasy_key_value_store::setValue(uint32_t key, const int32_t& value) { SET_4BYTE_TYPE(int32_type, setInt32); }

bool ESPEasy_key_value_store::getValue(uint32_t key, uint32_t& value) const
{
  GET_4BYTE_TYPE(uint32_type, getUint32);
}

void ESPEasy_key_value_store::setValue(uint32_t key, const uint32_t& value) { SET_4BYTE_TYPE(uint32_type, setUint32); }

bool ESPEasy_key_value_store::getValue(uint32_t key, int64_t& value) const
{
  GET_8BYTE_TYPE(int64_type, getInt64);
}

void ESPEasy_key_value_store::setValue(uint32_t key, const int64_t& value) { SET_8BYTE_TYPE(int64_type, setInt64); }

bool ESPEasy_key_value_store::getValue(uint32_t key, uint64_t& value) const
{
  GET_8BYTE_TYPE(uint64_type, getUint64);
}

void ESPEasy_key_value_store::setValue(uint32_t key, const uint64_t& value) { SET_8BYTE_TYPE(uint64_type, setUint64); }

bool ESPEasy_key_value_store::getValue(uint32_t key, float& value) const
{
  GET_4BYTE_TYPE(float_type, getFloat);
}

void ESPEasy_key_value_store::setValue(uint32_t key, const float& value) { SET_4BYTE_TYPE(float_type, setFloat); }

bool ESPEasy_key_value_store::getValue(uint32_t key, double& value) const
{
  GET_8BYTE_TYPE(double_type, getDouble);
}

void ESPEasy_key_value_store::setValue(uint32_t key, const double& value) { SET_8BYTE_TYPE(double_type, setDouble); }

bool ESPEasy_key_value_store::getValue(uint32_t key, bool& value) const
{
  GET_4BYTE_TYPE(bool_type, getUint32);
}

void ESPEasy_key_value_store::setValue(uint32_t key, const bool& value) { SET_4BYTE_TYPE(bool_type, setUint32); }


#define GET_4BYTE_TYPE_AS_STRING(T, CT)                 \
          case ESPEasy_key_value_store::StorageType::T: \
            {                                           \
              CT v{};                                   \
              if (getValue(key, v)) {                   \
                value = String(v);                      \
                return true;                            \
              }                                         \
              break;                                    \
            }

bool ESPEasy_key_value_store::getValue(StorageType& storageType, uint32_t key, String& value) const
{
  if (!hasKey(storageType, key)) { return false; }

  switch (storageType)
  {
    case ESPEasy_key_value_store::StorageType::string_type:
      getValue(key, value);
      break;
    GET_4BYTE_TYPE_AS_STRING(bool_type,   bool)
    GET_4BYTE_TYPE_AS_STRING(int8_type,   int8_t)
    GET_4BYTE_TYPE_AS_STRING(uint8_type,  uint8_t)
    GET_4BYTE_TYPE_AS_STRING(int16_type,  int16_t)
    GET_4BYTE_TYPE_AS_STRING(uint16_type, uint16_t)
    GET_4BYTE_TYPE_AS_STRING(int32_type,  int32_t)
    GET_4BYTE_TYPE_AS_STRING(uint32_type, uint32_t)
    GET_4BYTE_TYPE_AS_STRING(float_type,  float)

    case ESPEasy_key_value_store::StorageType::int64_type:
    {
      int64_t v{};

      if (getValue(key, v)) {
        value = ll2String(v);
        return true;
      }
      break;
    }

    case ESPEasy_key_value_store::StorageType::uint64_type:
    {
      uint64_t v{};

      if (getValue(key, v)) {
        value = ull2String(v);
        return true;
      }
      break;
    }

    case ESPEasy_key_value_store::StorageType::double_type:
    {
      double v{};

      if (getValue(key, v)) {
        value = doubleToString(v);
        return true;
      }
      break;
    }

    case ESPEasy_key_value_store::StorageType::not_set:
    case ESPEasy_key_value_store::StorageType::bool_true:
    case ESPEasy_key_value_store::StorageType::bool_false:
    case ESPEasy_key_value_store::StorageType::MAX_Type:
      break;
  }
  return false;
}

// TODO TD-er: Implement
void ESPEasy_key_value_store::setValue(StorageType& storageType, uint32_t key, const String& value) {}

bool ESPEasy_key_value_store::hasStorageType(StorageType storageType) const
{
  const uint32_t bitnr         = static_cast<uint32_t>(storageType);
  constexpr uint32_t max_bitnr = static_cast<uint32_t>(StorageType::MAX_Type);

  if (bitnr >= max_bitnr) { return false; }
  return bitRead(_storage_type_present_cache, bitnr);
}

void ESPEasy_key_value_store::setHasStorageType(StorageType storageType)
{
  const uint32_t bitnr         = static_cast<uint32_t>(storageType);
  constexpr uint32_t max_bitnr = static_cast<uint32_t>(StorageType::MAX_Type);

  if (bitnr < max_bitnr) {
    bitSet(_storage_type_present_cache, bitnr);
  }
}
