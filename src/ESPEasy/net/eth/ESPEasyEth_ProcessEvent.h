#pragma once

#include "../../../ESPEasy_common.h"

#if FEATURE_ETHERNET

namespace ESPEasy {
namespace net {
namespace eth {

void handle_unprocessedEthEvents();

void check_Eth_DNS_valid();

void processEthernetConnected();
void processEthernetDisconnected();
void processEthernetGotIP();
# if FEATURE_USE_IPV6
void processEthernetGotIPv6();
# endif

} // namespace eth
} // namespace net
} // namespace ESPEasy

#endif // if FEATURE_ETHERNET
