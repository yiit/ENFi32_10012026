#include "../net/_NWPlugin_Helper.h"

#if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS

# include "../../src/CustomBuild/ESPEasyLimits.h"

# include "../net/Globals/NWPlugins.h"
# include "../../src/Globals/Settings.h"

namespace ESPEasy {
namespace net {


NWPluginData_base *NWPlugin_task_data[NETWORK_MAX];

void resetNWPluginData() {
  for (ESPEasy::net::networkIndex_t i = 0; i < NR_ELEMENTS(NWPlugin_task_data); ++i) {
    NWPlugin_task_data[i] = nullptr;
  }
}

void clearNWPluginData(ESPEasy::net::networkIndex_t networkIndex) {
  if (validNetworkIndex(networkIndex)) {
    if (NWPlugin_task_data[networkIndex] != nullptr) {
      delete NWPlugin_task_data[networkIndex];
      NWPlugin_task_data[networkIndex] = nullptr;
    }
  }
}

bool initNWPluginData(ESPEasy::net::networkIndex_t networkIndex, NWPluginData_base *data) {
  if (!validNetworkIndex(networkIndex)) {
    if (data != nullptr) {
      delete data;
      data = nullptr;
    }
    return false;
  }

  // 2nd heap may have been active to allocate the NWPluginData, but here we need to keep the default heap active
  # ifdef USE_SECOND_HEAP
  HeapSelectDram ephemeral;
  # endif // ifdef USE_SECOND_HEAP


  clearNWPluginData(networkIndex);

  if (data != nullptr) {
    if (Settings.getNetworkEnabled(networkIndex)) {
      if (!data->init_KVS()) {
        delete data;
        data = nullptr;
      } else {
        NWPlugin_task_data[networkIndex] = data;
      }
    } else {
      delete data;
      data = nullptr;
    }
  }
  return getNWPluginData(networkIndex) != nullptr;
}

NWPluginData_base* getNWPluginData(ESPEasy::net::networkIndex_t networkIndex) {
  if (nwpluginTaskData_initialized(networkIndex)) {

    if (!NWPlugin_task_data[networkIndex]->baseClassOnly()) {
      return NWPlugin_task_data[networkIndex];
    }
  }
  return nullptr;
}

NWPluginData_base* getNWPluginDataBaseClassOnly(ESPEasy::net::networkIndex_t networkIndex) {
  if (nwpluginTaskData_initialized(networkIndex)) {
    return NWPlugin_task_data[networkIndex];
  }
  return nullptr;
}

bool nwpluginTaskData_initialized(ESPEasy::net::networkIndex_t networkIndex) {
  if (!validNetworkIndex(networkIndex)) {
    return false;
  }
  return NWPlugin_task_data[networkIndex] != nullptr &&
         (NWPlugin_task_data[networkIndex]->getNWPluginID() == Settings.getNWPluginID_for_network(networkIndex));
}

} // namespace net
} // namespace ESPEasy

#endif // if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS
