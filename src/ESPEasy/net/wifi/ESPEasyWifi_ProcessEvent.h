#pragma once

#include "../../../ESPEasy_common.h"

#if FEATURE_WIFI
namespace ESPEasy {
namespace net {
namespace wifi {

//void handle_unprocessedNetworkEvents();
//void processDisconnect();
//void processConnect();
//void processGotIP();
# if FEATURE_USE_IPV6
//void processGotIPv6();
# endif
//void processDisconnectAPmode();
//void processConnectAPmode();
//void processDisableAPmode();
//void processScanDone();

} // namespace wifi
} // namespace net
} // namespace ESPEasy

#endif // if FEATURE_WIFI
