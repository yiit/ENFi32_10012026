#pragma once

#include "../_NWPlugin_Helper.h"
#ifdef USES_NW001

# if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS

namespace ESPEasy {
namespace net {
namespace wifi {

struct NW001_data_struct_WiFi_STA : public NWPluginData_base {

};


} // namespace wifi
} // namespace net
} // namespace ESPEasy

# endif // if FEATURE_STORE_NETWORK_INTERFACE_SETTINGS
#endif // ifdef USES_NW001
