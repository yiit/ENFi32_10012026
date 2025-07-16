#ifndef WEBSERVER_ESPEASY_KEY_VALUE_STORE_WEBFORM_H
#define WEBSERVER_ESPEASY_KEY_VALUE_STORE_WEBFORM_H

#include "../WebServer/common.h"

#if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS

# include "../DataTypes/FormSelectorOptions.h"
# include "../Helpers/_ESPEasy_key_value_store.h"

struct WebFormItemParams {
  WebFormItemParams(const String                       & label,
                    const String                       & id,
                    ESPEasy_key_value_store::StorageType storageType,
                    uint32_t                             key = 0);

  WebFormItemParams(const __FlashStringHelper           *label,
                    const __FlashStringHelper           *id,
                    ESPEasy_key_value_store::StorageType storageType,
                    uint32_t                             key = 0);


  void checkRanges();

  String _label;
  String _id;
# if FEATURE_TOOLTIPS
  String _tooltip;
# endif // if FEATURE_TOOLTIPS
  float                                _min = INT_MIN;
  float                                _max = INT_MAX;
  uint8_t                              _nrDecimals{};
  float                                _stepsize    = 1;
  ESPEasy_key_value_store::StorageType _storageType = ESPEasy_key_value_store::StorageType::not_set;
  uint32_t                             _key{};
  int                                  _maxLength{};
  bool                                 _disabled{};
  bool                                 _readOnly{};
  bool                                 _required{};
  String                               _pattern;

  int64_t _defaultIntValue{};
  double  _defaultFloatValue{};
  String  _defaultStringValue;


};

bool showWebformItem(const ESPEasy_key_value_store& store,
                     WebFormItemParams              params);

void showFormSelector(const ESPEasy_key_value_store& store,
                      FormSelectorOptions          & selector,
                      const WebFormItemParams      & params);

void storeWebformItem(ESPEasy_key_value_store            & store,
                      uint32_t                             key,
                      ESPEasy_key_value_store::StorageType storageType,
                      const __FlashStringHelper           *id);

void storeWebformItem(ESPEasy_key_value_store            & store,
                      uint32_t                             key,
                      ESPEasy_key_value_store::StorageType storageType,
                      const String                       & id = EMPTY_STRING);

#endif // if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS
#endif // ifndef WEBSERVER_ESPEASY_KEY_VALUE_STORE_WEBFORM_H
