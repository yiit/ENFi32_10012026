#include "../Helpers/_NWPlugin_Helper_webform.h"

#include "../../ESPEasy_common.h"

#include "../DataStructs/ESPEasy_EventStruct.h"
#include "../DataTypes/ESPEasy_plugin_functions.h"
#include "../Globals/NWPlugins.h"
#include "../Globals/SecuritySettings.h"
#include "../Globals/Settings.h"
#include "../Helpers/Networking.h"
#include "../WebServer/ESPEasy_WebServer.h"
#include "../WebServer/Markup.h"
#include "../WebServer/Markup_Forms.h"

/*********************************************************************************************\
* Functions to load and store network settings on the web page.
\*********************************************************************************************/
const __FlashStringHelper* toString(NetworkSettingsStruct::VarType parameterIdx, bool displayName)
{
  switch (parameterIdx)
  {
    case NetworkSettingsStruct::NETWORK_IP:                       return F("Network IP");
    case NetworkSettingsStruct::NETWORK_PORT:                     return F("Network Port");
    case NetworkSettingsStruct::NETWORK_USER:                     return F("Network User");
    case NetworkSettingsStruct::NETWORK_PASS:                     return F("Network Password");
    case NetworkSettingsStruct::NETWORK_ENABLED:

      if (displayName) { return F("Enabled"); }
      else {             return F("networkenabled"); }


    default:
      return F("Undefined");
  }
}

String getNetworkParameterName(networkDriverIndex_t           NetworkDriverIndex,
                               NetworkSettingsStruct::VarType parameterIdx,
                               bool                           displayName,
                               bool                         & isAlternative) {
  String name;

  if (displayName) {
    EventStruct tmpEvent;
    tmpEvent.idx = parameterIdx;

    if (NWPluginCall(NetworkDriverIndex, NWPlugin::Function::NWPLUGIN_GET_PARAMETER_DISPLAY_NAME, &tmpEvent, name)) {
      // Found an alternative name for it.
      isAlternative = true;
      return name;
    }
  }
  isAlternative = false;

  name = toString(parameterIdx, displayName);

  if (!displayName) {
    // Change name to lower case and remove spaces to make it an internal name.
    name.toLowerCase();
    removeChar(name, ' ');
  }
  return name;
}

String getNetworkParameterInternalName(networkDriverIndex_t NetworkDriverIndex, NetworkSettingsStruct::VarType parameterIdx) {
  bool isAlternative; // Dummy, not needed for internal name
  bool displayName = false;

  return getNetworkParameterName(NetworkDriverIndex, parameterIdx, displayName, isAlternative);
}

String getNetworkParameterDisplayName(networkDriverIndex_t           NetworkDriverIndex,
                                      NetworkSettingsStruct::VarType parameterIdx,
                                      bool                         & isAlternative) {
  bool displayName = true;

  return getNetworkParameterName(NetworkDriverIndex, parameterIdx, displayName, isAlternative);
}

void addNetworkEnabledForm(networkIndex_t networkindex) {
  networkDriverIndex_t NetworkDriverIndex = getNetworkDriverIndex_from_NetworkIndex(networkindex);

  if (!validNetworkDriverIndex(NetworkDriverIndex)) {
    return;
  }

  NetworkSettingsStruct::VarType varType = NetworkSettingsStruct::NETWORK_ENABLED;

  bool isAlternativeDisplayName = false;
  const String displayName      = getNetworkParameterDisplayName(NetworkDriverIndex, varType, isAlternativeDisplayName);
  const String internalName     = getNetworkParameterInternalName(NetworkDriverIndex, varType);
  addFormCheckBox(displayName, internalName, Settings.getNetworkEnabled(networkindex));
}

void addNetworkParameterForm(const NetworkSettingsStruct  & NetworkSettings,
                             networkIndex_t                 networkindex,
                             NetworkSettingsStruct::VarType varType) {
  networkDriverIndex_t NetworkDriverIndex = getNetworkDriverIndex_from_NetworkIndex(networkindex);

  if (!validNetworkDriverIndex(NetworkDriverIndex)) {
    return;
  }

  bool isAlternativeDisplayName = false;
  const String displayName      = getNetworkParameterDisplayName(NetworkDriverIndex, varType, isAlternativeDisplayName);
  const String internalName     = getNetworkParameterInternalName(NetworkDriverIndex, varType);

  switch (varType)
  {
    case NetworkSettingsStruct::NETWORK_IP:
    {
      addFormIPBox(displayName, internalName, NetworkSettings.IP);
      break;
    }
    case NetworkSettingsStruct::NETWORK_PORT:
    {
      addFormNumericBox(displayName, internalName, NetworkSettings.Port, 1, 65535);
      break;
    }

    case NetworkSettingsStruct::NETWORK_USER:
    {
      /*
            const size_t fieldMaxLength =
              NetworkSettings.useExtendedCredentials() ? EXT_SECURITY_MAX_USER_LENGTH : sizeof(SecuritySettings.NetworkUser[0]) - 1;
            addFormTextBox(displayName,
                           internalName,
                           getNetworkUser(networkindex, NetworkSettings, false),
                           fieldMaxLength);
       */
      break;
    }
    case NetworkSettingsStruct::NETWORK_PASS:
    {
      /*
            const size_t fieldMaxLength =
              NetworkSettings.useExtendedCredentials() ? EXT_SECURITY_MAX_PASS_LENGTH : sizeof(SecuritySettings.NetworkPassword[0]) - 1;

            if (isAlternativeDisplayName) {
              // It is not a regular password, thus use normal text field.
              addFormTextBox(displayName, internalName,
                             getNetworkPass(networkindex, NetworkSettings),
                             fieldMaxLength);
            } else {
              addFormPasswordBox(displayName, internalName,
                                 getNetworkPass(networkindex, NetworkSettings),
                                 fieldMaxLength);
            }
       */
      break;
    }

    case NetworkSettingsStruct::NETWORK_ENABLED:
      addFormCheckBox(displayName, internalName, Settings.getNetworkEnabled(networkindex));
      break;
  }
}

void saveNetworkParameterForm(NetworkSettingsStruct        & NetworkSettings,
                              networkIndex_t                 networkindex,
                              NetworkSettingsStruct::VarType varType) {
  const networkDriverIndex_t NetworkDriverIndex =
    getNetworkDriverIndex_from_NetworkIndex(networkindex);

  if (!validNetworkDriverIndex(NetworkDriverIndex)) {
    return;
  }
  const String internalName = getNetworkParameterInternalName(NetworkDriverIndex, varType);

  switch (varType)
  {

    case NetworkSettingsStruct::NETWORK_IP:
      /*
            if (!NetworkSettings.UseDNS)
            {
              str2ip(webArg(internalName), NetworkSettings.IP);
            }
       */
      break;
    case NetworkSettingsStruct::NETWORK_PORT:
      NetworkSettings.Port = getFormItemInt(internalName, NetworkSettings.Port);
      break;

    case NetworkSettingsStruct::NETWORK_USER:
      //      setNetworkUser(networkindex, NetworkSettings, webArg(internalName));
      break;
    case NetworkSettingsStruct::NETWORK_PASS:
    {
      String password;

      if (getFormPassword(internalName, password)) {
        //        setNetworkPass(networkindex, NetworkSettings, password);
      }
      break;
    }

    case NetworkSettingsStruct::NETWORK_ENABLED:
      Settings.setNetworkEnabled(networkindex, isFormItemChecked(internalName));
      addLog(LOG_LEVEL_INFO,
             concat(F("Save NW_Enabled: "),
                    internalName) + ' ' + isFormItemChecked(internalName) + ' ' + Settings.getNetworkEnabled(networkindex));
      break;
  }
}

#ifdef ESP32
bool print_IP_address(NWPlugin::IP_type ip_type, NetworkInterface* networkInterface, Print &out)
{
  const IPAddress ip(get_IP_address(ip_type, networkInterface));
  const size_t nrBytes = ip.printTo(out, true) > 0;
  if (ip.type() == IPv4) {
    const uint32_t val(ip);
    if (val == 0) return false;
  }
  return nrBytes > 0;
}

IPAddress get_IP_address(NWPlugin::IP_type ip_type, NetworkInterface*networkInterface)
{
  IPAddress ip;

  if (networkInterface) {
# if CONFIG_LWIP_IPV6
    esp_ip6_addr_type_t ip6_addr_type = ESP_IP6_ADDR_IS_UNKNOWN;
#endif
    switch (ip_type)
    {
      case NWPlugin::IP_type::inet:        return networkInterface->localIP();
      case NWPlugin::IP_type::netmask:     return networkInterface->subnetMask();
      case NWPlugin::IP_type::broadcast:   return networkInterface->broadcastIP();
      case NWPlugin::IP_type::gateway:     return networkInterface->gatewayIP();
      case NWPlugin::IP_type::dns1:        return networkInterface->dnsIP(0);
      case NWPlugin::IP_type::dns2:        return networkInterface->dnsIP(1);
# if CONFIG_LWIP_IPV6
      case NWPlugin::IP_type::ipv6_unknown:      ip6_addr_type = ESP_IP6_ADDR_IS_UNKNOWN; break;
      case NWPlugin::IP_type::ipv6_global:       ip6_addr_type = ESP_IP6_ADDR_IS_GLOBAL; break;
      case NWPlugin::IP_type::ipv6_link_local:   ip6_addr_type = ESP_IP6_ADDR_IS_LINK_LOCAL; break;
      case NWPlugin::IP_type::ipv6_site_local:   ip6_addr_type = ESP_IP6_ADDR_IS_SITE_LOCAL; break;
      case NWPlugin::IP_type::ipv6_unique_local: ip6_addr_type = ESP_IP6_ADDR_IS_UNIQUE_LOCAL; break;
      case NWPlugin::IP_type::ipv4_mapped_ipv6:  ip6_addr_type = ESP_IP6_ADDR_IS_IPV4_MAPPED_IPV6; break;
# endif // if CONFIG_LWIP_IPV6
      default: 
        return ip;
    }
# if CONFIG_LWIP_IPV6
    if (networkInterface->netif()) {
      esp_ip6_addr_t if_ip6[CONFIG_LWIP_IPV6_NUM_ADDRESSES];
      int v6addrs = esp_netif_get_all_ip6(networkInterface->netif(), if_ip6);
      for (int i = 0; i < v6addrs; ++i) {
        if (esp_netif_ip6_get_addr_type(&if_ip6[i]) == ip6_addr_type) {
          return IPAddress(IPv6, (const uint8_t *)if_ip6[i].addr, if_ip6[i].zone);
        }
      }
    }
# endif
  }

  return ip;
}

#endif // ifdef ESP32
