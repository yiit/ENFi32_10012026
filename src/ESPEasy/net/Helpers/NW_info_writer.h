#pragma once

#include "../../../ESPEasy_common.h"

#include "../../../src/Helpers/KeyValueWriter.h"

#include "../DataTypes/NetworkIndex.h"

namespace ESPEasy {
namespace net {

#ifdef ESP32
bool write_NetworkAdapterFlags(ESPEasy::net::networkIndex_t networkindex,
                               KeyValueWriter              *writer);

bool write_NetworkAdapterPort(ESPEasy::net::networkIndex_t networkindex,
                               KeyValueWriter              *writer);

bool write_IP_config(ESPEasy::net::networkIndex_t networkindex,
                     KeyValueWriter              *writer);
#endif // ifdef ESP32

bool write_NetworkConnectionInfo(ESPEasy::net::networkIndex_t networkindex,
                                 KeyValueWriter              *writer);

} // namespace net
} // namespace ESPEasy
