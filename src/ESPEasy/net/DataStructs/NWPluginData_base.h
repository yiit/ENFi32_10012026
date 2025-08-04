#pragma once

#include "../../../ESPEasy_common.h"


#include "../DataTypes/NWPluginID.h"
#include "../DataTypes/NetworkIndex.h"
#include "../../../src/DataStructs/ESPEasy_EventStruct.h"
#include "../DataStructs/NWPluginData_static_runtime.h"

#if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS
# include "../../../src/Helpers/_ESPEasy_key_value_store.h"
#endif

namespace ESPEasy {
namespace net {


// =============================================
// Data used by instances of NW-plugins.
// =============================================

// base class to be able to delete a data object from the array.
// N.B. in order to use this, a data object must inherit from this base class.
//      This is a compile time check.
struct NWPluginData_base {
  NWPluginData_base(nwpluginID_t      nwpluginID,
                    networkIndex_t networkIndex
#ifdef                                ESP32
                    ,
                    NetworkInterface *netif
#endif // ifdef ESP32
                    );

  virtual ~NWPluginData_base();

  virtual bool init(EventStruct *event) = 0;

  virtual bool exit(EventStruct *event) = 0;

  bool         baseClassOnly() const {
    return _baseClassOnly;
  }

  bool plugin_write_base(EventStruct  *event,
                         const String& string);


  // Should only be called from initNWPluginData
#if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS
  bool                            init_KVS();
#endif

  nwpluginID_t                    getNWPluginID() const { return _nw_data_pluginID; }

  virtual LongTermTimer::Duration getConnectedDuration_ms();

#ifdef ESP32
  virtual bool                    handle_priority_route_changed();
  bool                            getTrafficCount(uint64_t& tx,
                                                  uint64_t& rx) const;
#endif // ifdef ESP32

  virtual NWPluginData_static_runtime& getNWPluginData_static_runtime() = 0;

protected:

#ifdef ESP32
  bool _restore_DNS_cache();
#endif

#if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS

  bool _KVS_initialized() const { return _kvs != nullptr; }

  // Load settings in the _kvs
  bool _load();

  // Save settings from the _kvs to the settings
  bool _store();

  ESPEasy_key_value_store *_kvs = nullptr;
#endif // if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS

  // We cannot use dynamic_cast, so we must keep track of the plugin ID to
  // perform checks on the casting.
  // This is also a check to only use these functions and not to insert pointers
  // at random in the Plugin_task_data array.
  nwpluginID_t   _nw_data_pluginID = INVALID_NW_PLUGIN_ID;
  networkIndex_t _networkIndex = INVALID_NETWORK_INDEX;

#if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS
  bool _baseClassOnly = false;
#else
  bool _baseClassOnly = true;
#endif // if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS

#ifdef ESP32
  NetworkInterface *_netif{};
#endif

};

} // namespace net
} // namespace ESPEasy
