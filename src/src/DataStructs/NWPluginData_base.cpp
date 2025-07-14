#include "../DataStructs/NWPluginData_base.h"

#include "../DataStructs/ESPEasy_EventStruct.h"

#include "../Globals/RuntimeData.h"

#include "../Helpers/StringConverter.h"
#include "../Helpers/_ESPEasy_key_value_store.h"



NWPluginData_base::~NWPluginData_base()
{
  if (_kvs) { delete _kvs; }
  _kvs = nullptr;
}

bool NWPluginData_base::plugin_write_base(struct EventStruct *event,
                                          const String      & string) { return false; }

bool NWPluginData_base::init_KVS()
{
    _kvs = new (std::nothrow) ESPEasy_key_value_store;
    return _kvs != nullptr;
}

bool NWPluginData_base::_load()
{
  if (!_kvs) { return false; }
  return _kvs->load(
    SettingsType::Enum::NetworkInterfaceSettings_Type,
    _networkIndex,
    0);
}

bool NWPluginData_base::_store()
{
  if (!_kvs) { return false; }
  return _kvs->store(
    SettingsType::Enum::NetworkInterfaceSettings_Type,
    _networkIndex,
    0);
}
