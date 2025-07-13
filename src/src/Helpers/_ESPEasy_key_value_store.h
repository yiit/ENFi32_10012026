#ifndef HELPERS_ESPEASY_KEY_VALUE_STORE_H
#define HELPERS_ESPEASY_KEY_VALUE_STORE_H

#include "../../ESPEasy_common.h"

#include "../DataTypes/ESPEasy_key_value_store_data.h"
#include "../DataTypes/SettingsType.h"

#include <map>

// Generic storage layer, which on a low level will store everything as a string.
// Low level storage structure:
// - uint8_t:    version of storage layer
// - uint8_t[3]: rawSize (offset to start of checksum)
// - List of key/value pairs:
//   - uint8_t:    storage type
//   - uint8_t[3]: key
//   - zero-terminated string or shortened binary form
//   ...
// - uint8_t[16] checksum

class ESPEasy_key_value_store
{
public:

  // Type is stored, so do not change the order/values
  enum class StorageType {
    not_set     = 0,
    string_type = 1,
    int8_type   = 2,
    uint8_type  = 3,
    int16_type  = 4,
    uint16_type = 5,
    int32_type  = 6,
    uint32_type = 7,
    int64_type  = 8,
    uint64_type = 9,
    float_type  = 10,
    double_type = 11,
    bool_type   = 12,

    MAX_Type,         // Leave this one after the generic type and before 'special' types


    bool_true  = 254, // small optimization to store 'true' as extra type
    bool_false = 255  // small optimization to store 'false' as extra type

  };


  bool   load(SettingsType::Enum settingsType, int index, uint32_t offset_in_block); 
  bool   store(SettingsType::Enum settingsType, int index, uint32_t offset_in_block); 

  // Count all data to estimate how much storage space it would require to store everything in a somewhat compact form.
  size_t getPayloadStorageSize() const;

  // Check to see if there is a key stored with given storage type
  bool   hasKey(StorageType storageType,
                uint32_t    key) const;

  // Try to find a key and get its storage type or 'not_set' if the key is not present
  StorageType getStorageType(uint32_t key) const;


  /*
   ###############################################
   ## get/set functions for all supported types ##
   ###############################################
   */

  bool getValue(uint32_t key,
                String & value) const;
  void setValue(uint32_t      key,
                const String& value);
  void setValue(uint32_t      key,
                String&& value);

  bool getValue(uint32_t key,
                int8_t & value) const;
  void setValue(uint32_t      key,
                const int8_t& value);

  bool getValue(uint32_t key,
                uint8_t& value) const;
  void setValue(uint32_t       key,
                const uint8_t& value);

  bool getValue(uint32_t key,
                int16_t& value) const;
  void setValue(uint32_t       key,
                const int16_t& value);

  bool getValue(uint32_t  key,
                uint16_t& value) const;
  void setValue(uint32_t        key,
                const uint16_t& value);

  bool getValue(uint32_t key,
                int32_t& value) const;
  void setValue(uint32_t       key,
                const int32_t& value);

  bool getValue(uint32_t  key,
                uint32_t& value) const;
  void setValue(uint32_t        key,
                const uint32_t& value);

  bool getValue(uint32_t key,
                int64_t& value) const;
  void setValue(uint32_t       key,
                const int64_t& value);

  bool getValue(uint32_t  key,
                uint64_t& value) const;
  void setValue(uint32_t        key,
                const uint64_t& value);

  bool getValue(uint32_t key,
                float  & value) const;
  void setValue(uint32_t     key,
                const float& value);

  bool getValue(uint32_t key,
                double & value) const;
  void setValue(uint32_t      key,
                const double& value);

  bool getValue(uint32_t key,
                bool   & value) const;
  void setValue(uint32_t    key,
                const bool& value);


  // Generic get function for any given storageType/key and represent its value as a string.
  // Return false when storageType/key is not present.
  bool getValue(StorageType& storageType,
                uint32_t     key,
                String     & value) const;

  // Generic set function for any given storageType/key.
  // Given value is a string representation of that storage type.
  // TODO TD-er: Implement
  void setValue(StorageType & storageType,
                uint32_t      key,
                const String& value);

  String getLastError() const { return _lastError; }

private:

  bool getValue(StorageType& storageType,
                uint32_t     key,
                ESPEasy_key_value_store_4byte_data_t& value) const;

  void setValue(StorageType & storageType,
                uint32_t      key,
                const ESPEasy_key_value_store_4byte_data_t& value);

  bool getValue(StorageType& storageType,
                uint32_t     key,
                ESPEasy_key_value_store_8byte_data_t& value) const;

  void setValue(StorageType & storageType,
                uint32_t      key,
                const ESPEasy_key_value_store_8byte_data_t& value);



  // Query cache to see if we have any of the asked storage type
  bool hasStorageType(StorageType storageType) const;

  // Update cache to indicate we have at least one of the given storage type
  void setHasStorageType(StorageType storageType);

  String _lastError;

  std::map<uint32_t, String> _string_data{};
  std::map<uint32_t, ESPEasy_key_value_store_4byte_data_t>_4byte_data{};
  std::map<uint32_t, ESPEasy_key_value_store_8byte_data_t>_8byte_data{};

  uint32_t _storage_type_present_cache{};

  // TODO TD-er: Add checksum and invalidate whenever anyting is being stored.

}; // class ESPEasy_key_value_store

#endif // ifndef HELPERS_ESPEASY_KEY_VALUE_STORE_H
