#include "../DataStructs/NWPluginData_base.h"

#if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS

# include "../DataStructs/ESPEasy_EventStruct.h"
# include "../Globals/RuntimeData.h"
# include "../Globals/Settings.h"
# include "../Helpers/StringConverter.h"
# include "../Helpers/_ESPEasy_key_value_store.h"


NWPluginData_base::NWPluginData_base(nwpluginID_t nwpluginID, networkIndex_t networkIndex) :
  _kvs(nullptr),
  _nw_data_pluginID(nwpluginID),
  _networkIndex(networkIndex),
  _baseClassOnly(false)
{
  if (_kvs == nullptr) {
    _kvs = new (std::nothrow) ESPEasy_key_value_store;
  }
}

NWPluginData_base::~NWPluginData_base()
{
  if (_kvs) { delete _kvs; }
  _kvs = nullptr;
}

bool NWPluginData_base::plugin_write_base(struct EventStruct *event,
                                          const String      & string) { return false; }

bool NWPluginData_base::init_KVS()
{ 
  if (!_KVS_initialized()) { return false; }
//  _load();

  // TODO TD-er: load() can also return false when some other data used to be present. Have to think about how to handle this.
  return true;
}

bool NWPluginData_base::_load()
{
  if (!_kvs) { return false; }
  return _kvs->load(
    SettingsType::Enum::NetworkInterfaceSettings_Type,
    _networkIndex,
    0,
    _nw_data_pluginID.value);
}

bool NWPluginData_base::_store()
{
  if (!_kvs) { return false; }
  return _kvs->store(
    SettingsType::Enum::NetworkInterfaceSettings_Type,
    _networkIndex,
    0,
    _nw_data_pluginID.value);
}

#endif // if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS
