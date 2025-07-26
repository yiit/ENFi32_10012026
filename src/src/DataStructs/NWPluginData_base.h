#ifndef DATASTRUCTS_NWPLUGINDATA_BASE_H
#define DATASTRUCTS_NWPLUGINDATA_BASE_H

#include "../../ESPEasy_common.h"

#if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS

# include "../DataStructs/ESPEasy_EventStruct.h"
# include "../DataTypes/NWPluginID.h"
# include "../DataTypes/NetworkIndex.h"
# include "../Helpers/_ESPEasy_key_value_store.h"

namespace ESPEasy {
namespace net {


// ==============================================
// Data used by instances of NW-plugins.
// =============================================

// base class to be able to delete a data object from the array.
// N.B. in order to use this, a data object must inherit from this base class.
//      This is a compile time check.
struct NWPluginData_base {
  NWPluginData_base(nwpluginID_t   nwpluginID,
                    networkIndex_t networkIndex);

  virtual ~NWPluginData_base();

  bool baseClassOnly() const {
    return _baseClassOnly;
  }

  bool plugin_write_base(EventStruct  *event,
                         const String& string);


  // Should only be called from initNWPluginData
  bool         init_KVS();

  nwpluginID_t getNWPluginID() const { return _nw_data_pluginID; }

protected:

  bool _KVS_initialized() const { return _kvs != nullptr; }

  // Load settings in the _kvs
  bool _load();

  // Save settings from the _kvs to the settings
  bool _store();

  ESPEasy_key_value_store *_kvs = nullptr;

  // We cannot use dynamic_cast, so we must keep track of the plugin ID to
  // perform checks on the casting.
  // This is also a check to only use these functions and not to insert pointers
  // at random in the Plugin_task_data array.
  nwpluginID_t   _nw_data_pluginID = INVALID_NW_PLUGIN_ID;
  networkIndex_t _networkIndex     = INVALID_NETWORK_INDEX;

  bool _baseClassOnly = false;

};

} // namespace net
} // namespace ESPEasy

#endif // if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS

#endif // ifndef DATASTRUCTS_NWPLUGINDATA_BASE_H
