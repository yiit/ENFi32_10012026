#include "../DataTypes/ESPEasy_plugin_functions.h"

const __FlashStringHelper * NWPlugin::toString(NWPlugin::IP_type ip_type)
{
  switch (ip_type)
  {
    case NWPlugin::IP_type::inet:              return F("Inet");
    case NWPlugin::IP_type::netmask:           return F("Netmask");
    case NWPlugin::IP_type::broadcast:         return F("Broadcast");
    case NWPlugin::IP_type::gateway:           return F("Gateway");
    case NWPlugin::IP_type::dns1:              return F("Dns1");
    case NWPlugin::IP_type::dns2:              return F("Dns2");
#if CONFIG_LWIP_IPV6
    case NWPlugin::IP_type::ipv6_unknown:      return F("IPv6 unknown");
    case NWPlugin::IP_type::ipv6_global:       return F("IPv6 global");
    case NWPlugin::IP_type::ipv6_link_local:   return F("IPv6 link local");
    case NWPlugin::IP_type::ipv6_site_local:   return F("IPv6 site local");
    case NWPlugin::IP_type::ipv6_unique_local: return F("IPv6 unique local");
    case NWPlugin::IP_type::ipv4_mapped_ipv6:  return F("IPv4 mapped IPv6");
#endif // if CONFIG_LWIP_IPV6
  }
  return F("unknown");
}
