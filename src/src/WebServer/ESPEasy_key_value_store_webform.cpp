#include "../WebServer/ESPEasy_key_value_store_webform.h"

#include "../Helpers/StringConverter.h"

#include "../WebServer/Markup_Forms.h"

WebFormItemParams::WebFormItemParams(
  const String                       & label,
  const String                       & id,
  ESPEasy_key_value_store::StorageType storageType)
  : _label(label), _id(id), _storageType(storageType) {}

WebFormItemParams::WebFormItemParams(
  const __FlashStringHelper           *label,
  const __FlashStringHelper           *id,
  ESPEasy_key_value_store::StorageType storageType)
  : _label(label), _id(id), _storageType(storageType) {}

bool showWebformItem(const ESPEasy_key_value_store& store,
                     uint32_t                       key,
                     const WebFormItemParams      & params)
{
  String id = params._id.isEmpty() ? concat(F("KVS_ID_"), key) : params._id;

  switch (params._storageType)
  {
    case ESPEasy_key_value_store::StorageType::string_type:
    {
      String value;

      if (!store.getValueAsString(key, value)) { return false; }
      addFormTextBox(
        params._label,
        id,
        value,
        params._maxLength,
        params._readOnly,
        params._required,
        params._pattern
#if FEATURE_TOOLTIPS
        , params._tooltip
#endif // if FEATURE_TOOLTIPS
        );
      return true;
    }
    case ESPEasy_key_value_store::StorageType::int8_type:
    case ESPEasy_key_value_store::StorageType::uint8_type:
    case ESPEasy_key_value_store::StorageType::int16_type:
    case ESPEasy_key_value_store::StorageType::uint16_type:
    case ESPEasy_key_value_store::StorageType::int32_type:
    case ESPEasy_key_value_store::StorageType::uint32_type:
    {
      String valueAsString;

      if (!store.getValueAsString(key, valueAsString)) { return false; }
      addFormNumericBox(
        params._label,
        id,
        valueAsString.toInt(),
        params._min,
        params._max,
#if FEATURE_TOOLTIPS
        params._tooltip,
#endif // if FEATURE_TOOLTIPS
        params._disabled
        );

      return true;
    }

    case ESPEasy_key_value_store::StorageType::int64_type:
    {
      break;
    }

    case ESPEasy_key_value_store::StorageType::uint64_type:
    {
      break;
    }

    case ESPEasy_key_value_store::StorageType::float_type:
    case ESPEasy_key_value_store::StorageType::double_type:
    {
      float value{};

      if (!store.getValue(key, value)) { return false; }
      addFormFloatNumberBox(
        params._label,
        id,
        value,
        params._min,
        params._max,
        params._nrDecimals,
        params._stepsize,

#if FEATURE_TOOLTIPS
        params._tooltip
#endif // if FEATURE_TOOLTIPS
        );


      return true;
    }

    case ESPEasy_key_value_store::StorageType::bool_type:
    {
      bool value{};

      if (!store.getValue(key, value)) { return false; }
      addFormCheckBox(
        params._label,
        id,
        value,
        params._disabled);
      return true;
    }

    default: break;
  }
  return false;
}

void storeWebformItem(ESPEasy_key_value_store            & store,
                      uint32_t                             key,
                      ESPEasy_key_value_store::StorageType storageType,
                      const __FlashStringHelper           *id) { storeWebformItem(store, key, storageType, String(id)); }

void storeWebformItem(ESPEasy_key_value_store            & store,
                      uint32_t                             key,
                      ESPEasy_key_value_store::StorageType storageType,
                      const String                       & id)
{
  String _id = id.isEmpty() ? concat(F("KVS_ID_"), key) : id;

  if (storageType == ESPEasy_key_value_store::StorageType::bool_type) {
    store.setValue(key, isFormItemChecked(_id));
    return;
  }
  store.setValue(storageType, key, webArg(_id));
}
