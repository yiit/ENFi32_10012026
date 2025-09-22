#ifndef ESPEASY_ETH_H
#define ESPEASY_ETH_H

#include "../../../ESPEasy_common.h"

#if FEATURE_ETHERNET

# include "../DataStructs/NWPluginData_static_runtime.h"
# include "../DataStructs/MAC_address.h"

# include <IPAddress.h>

namespace ESPEasy {
namespace net {
namespace eth {

bool        ethUseStaticIP();
void        ethSetupStaticIPconfig();
bool        ethCheckSettings();
bool        ethPrepare();
void        ethPrintSettings();
bool        ETHConnectRelaxed(NWPluginData_static_runtime& runtimeData);
bool        ETHConnected();
void        ethPower(NWPluginData_static_runtime& runtimeData, bool enable);
void        ethResetGPIOpins();
MAC_address ETHMacAddress();


} // namespace eth
} // namespace net
} // namespace ESPEasy

#endif // if FEATURE_ETHERNET
#endif // ifndef ESPEASY_ETH_H
