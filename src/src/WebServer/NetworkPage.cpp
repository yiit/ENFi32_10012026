#include "../WebServer/NetworkPage.h"

#ifdef WEBSERVER_NETWORK


# include "../DataStructs/ESPEasy_EventStruct.h"
# include "../Globals/Settings.h"
# include "../Helpers/ESPEasy_Storage.h"
# include "../Helpers/ESPEasy_Storage.h"
# include "../Helpers/PrintToString.h"
# include "../Helpers/StringConverter.h"
# include "../WebServer/ESPEasy_WebServer.h"
# include "../WebServer/HTML_wrappers.h"
# include "../WebServer/Markup.h"
# include "../WebServer/Markup_Buttons.h"
# include "../WebServer/Markup_Forms.h"
# include "../../ESPEasy/net/Globals/NWPlugins.h"
# include "../../ESPEasy/net/Helpers/_NWPlugin_Helper_webform.h"
# include "../../ESPEasy/net/Helpers/_NWPlugin_init.h"

using namespace ESPEasy::net;

void handle_networks()
{
# ifndef BUILD_NO_RAM_TRACKER
  checkRAM(F("handle_networks"));
# endif // ifndef BUILD_NO_RAM_TRACKER

  if (!isLoggedIn()) { return; }
  navMenuIndex = MENU_INDEX_NETWORK;
  TXBuffer.startStream();
  sendHeadandTail_stdtemplate(_HEAD);


  // 'index' value in the URL
  uint8_t networkindex  = getFormItemInt(F("index"), 0);
  boolean networkNotSet = networkindex == 0;
  --networkindex; // Index in URL is starting from 1, but starting from 0 in the array.

  const int networkDriver_webarg_value = getFormItemInt(F("networkDriver"), -1);

  // submitted data
  if ((networkDriver_webarg_value != -1) && !networkNotSet)
  {
    bool mustInit             = false;
    bool mustCallNWpluginSave = false;

    // TODO TD-er: Implement saving submitted settings
    const nwpluginID_t nwpluginID = nwpluginID_t::toPluginID(networkDriver_webarg_value);

    MakeNetworkSettings(NetworkSettings);

    if (Settings.getNWPluginID_for_network(networkindex) != nwpluginID)
    {
      addLog(LOG_LEVEL_INFO, strformat(
               F("HandleNW: Driver changed from %d to %d"),
               Settings.getNWPluginID_for_network(networkindex).value,
               nwpluginID.value));

      // NetworkDriver has changed.
      Settings.setNWPluginID_for_network(networkindex, nwpluginID);

      // there is a networkDriverIndex selected?
      if (nwpluginID.isValid())
      {
        mustInit = true;

        handle_networks_clearLoadDefaults(networkindex, *NetworkSettings);
      }
    }

    // subitted same networkDriverIndex
    else
    {
      // there is a networkDriverIndex selected
      addLog(LOG_LEVEL_INFO, concat(
               F("HandleNW: Driver selected: "),
               nwpluginID.value) + (nwpluginID.isValid() ? F("valid") : F("invalid")));

      if (nwpluginID.isValid())
      {
        mustInit = true;

        handle_networks_CopySubmittedSettings(networkindex, *NetworkSettings);
        mustCallNWpluginSave = true;
      }
    }

    if (mustCallNWpluginSave) {
      // Call NWPLUGIN_WEBFORM_SAVE after destructing NetworkSettings object to reduce RAM usage.
      // Network plugin almost only deals with custom network settings.
      // Even if they need to save things to the NetworkSettings, then the changes must
      // already be saved first as the NWPluginCall does not have the NetworkSettings as argument.
      handle_networks_CopySubmittedSettings_NWPluginCall(networkindex);
    }
    addHtmlError(SaveSettings());

    if (mustInit) {
      // Init network plugin using the new settings.
      networkDriverIndex_t NetworkDriverIndex = getNetworkDriverIndex_from_NWPluginID(nwpluginID);

      if (validNetworkDriverIndex(NetworkDriverIndex)) {
        struct EventStruct TempEvent;
        TempEvent.NetworkIndex = networkindex;
        String dummy;
        NWPlugin::Function nfunction =
          Settings.getNetworkEnabled(networkindex) ? NWPlugin::Function::NWPLUGIN_INIT : NWPlugin::Function::NWPLUGIN_EXIT;
        do_NWPluginCall(NetworkDriverIndex, nfunction, &TempEvent, dummy);
      }
    }

  }

  html_add_form();

  if (networkNotSet)
  {
    handle_networks_ShowAllNetworksTable();
  }
  else
  {
    handle_networks_NetworkSettingsPage(networkindex);
  }

  sendHeadandTail_stdtemplate(_TAIL);
  TXBuffer.endStream();
}

void handle_networks_clearLoadDefaults(ESPEasy::net::networkIndex_t networkindex, NetworkSettingsStruct& NetworkSettings) {
  networkDriverIndex_t NetworkDriverIndex = getNetworkDriverIndex_from_NetworkIndex(networkindex);

  if (validNetworkDriverIndex(NetworkDriverIndex)) {
    struct EventStruct TempEvent;
    TempEvent.NetworkIndex = networkindex;

    String dummy;
    do_NWPluginCall(
      NetworkDriverIndex,
      NWPlugin::Function::NWPLUGIN_LOAD_DEFAULTS, &TempEvent, dummy);
  }

  // TODO TD-er: Must also check NetworkDriverStruct to see if something else must be done
}

void handle_networks_CopySubmittedSettings(ESPEasy::net::networkIndex_t networkindex, NetworkSettingsStruct& NetworkSettings)
{
  // copy all settings to network settings struct
  for (int parameterIdx = 0; parameterIdx <= NetworkSettingsStruct::NETWORK_ENABLED; ++parameterIdx) {
    NetworkSettingsStruct::VarType varType = static_cast<NetworkSettingsStruct::VarType>(parameterIdx);
    saveNetworkParameterForm(NetworkSettings, networkindex, varType);
  }

}

void handle_networks_CopySubmittedSettings_NWPluginCall(ESPEasy::net::networkIndex_t networkindex) {
  networkDriverIndex_t NetworkDriverIndex = getNetworkDriverIndex_from_NetworkIndex(networkindex);

  if (validNetworkDriverIndex(NetworkDriverIndex)) {
    struct EventStruct TempEvent;
    TempEvent.NetworkIndex = networkindex;

    // Call network plugin to save CustomNetworkSettings
    addLog(LOG_LEVEL_INFO, F("Call network plugin to save CustomNetworkSettings"));
    String dummy;
    do_NWPluginCall(NetworkDriverIndex, NWPlugin::Function::NWPLUGIN_WEBFORM_SAVE, &TempEvent, dummy);
  }

}

void handle_networks_ShowAllNetworksTable()
{
  html_table_class_multirow();
  html_TR();
  html_table_header(F(""),        70);
  html_table_header(F("Enabled"), 100);
  html_table_header(F("Network Adapter"));
  #ifdef ESP32
  html_table_header(F("Prio"),   50);
  #endif
  html_table_header(F("Connected"));
  html_table_header(F("Hostname/SSID"));
  html_table_header(F("HW Address"));
  html_table_header(F("IP"));
  html_table_header(F("Port"));

  for (ESPEasy::net::networkIndex_t x = 0; x < NETWORK_MAX; x++)
  {
    const nwpluginID_t nwpluginID = Settings.getNWPluginID_for_network(x);
    const bool nwplugin_set       = nwpluginID.isValid();

    html_TR_TD();

    if (nwplugin_set && !supportedNWPluginID(nwpluginID)) {
      html_add_button_prefix(F("red"), true);
    } else {
      html_add_button_prefix();
    }
    {
      addHtml(F("network?index="));
      addHtmlInt(x + 1);
      addHtml(F("'>"));

      if (nwplugin_set) {
        addHtml(F("Edit"));
      } else {
        addHtml(F("Add"));
      }
      addHtml(F("</a><TD>"));
    }

    if (nwplugin_set)
    {
      addEnabled(Settings.getNetworkEnabled(x));
      html_TD();

      // Network Adapter
      addHtml(getNWPluginNameFromNWPluginID(Settings.getNWPluginID_for_network(x)));

      const NWPlugin::Function functions[] {
#ifdef ESP32
        NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_ROUTE_PRIO,
#endif
        NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_CONNECTED,
        NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_HOSTNAME,
        NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_HW_ADDRESS,
        NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_IP,
        NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_PORT
      };

      const networkDriverIndex_t NetworkDriverIndex = getNetworkDriverIndex_from_NetworkIndex(x);

      for (uint8_t i = 0; i < NR_ELEMENTS(functions); ++i) {
        html_TD();
        struct EventStruct TempEvent;
        TempEvent.NetworkIndex = x;

        String str;

        // const bool res = do_NWPluginCall(NetworkDriverIndex, functions[i], &TempEvent, str);
        const bool res = NWPluginCall(functions[i], &TempEvent, str);
#ifdef ESP32
        if (functions[i] == NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_ROUTE_PRIO) {
          if (TempEvent.Par1 > 0) {
            addHtmlInt(TempEvent.Par1);
            if (TempEvent.Par2) {
              addHtml(F("(*)"));
            }
          }
        } else 
#endif
        if (functions[i] == NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_CONNECTED) {
          if (!res || str.isEmpty()) {
            addEnabled(res);
          }
        } else if (functions[i] == NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_HW_ADDRESS) {
          if (res && !TempEvent.String1.isEmpty() && !str.isEmpty()) {
            addHtml(strformat(F("%s: "), TempEvent.String1.c_str()));
          }
        }

        if (!str.isEmpty()) {
          str.replace(F("\n"), F("<br>"));
          addHtml(str);
        }

      }

      /*
         const NetworkDriverStruct& driver = ESPEasy::net::getNetworkDriverStruct(NetworkDriverIndex);

         if ((INVALID_NETWORKDRIVER_INDEX == NetworkDriverIndex) || driver.usesPort) {
         addHtmlInt(13 == nwpluginID ? Settings.UDPPort : NetworkSettings->Port); // P2P/C013 exception
         }
       */

    }
    else {
      html_TD(7);
    }
  }

  html_end_table();
  html_end_form();
}

void handle_networks_NetworkSettingsPage(ESPEasy::net::networkIndex_t networkindex) {
  if (!validNetworkIndex(networkindex)) { return; }

  const networkDriverIndex_t networkDriverIndex =
    getNetworkDriverIndex_from_NWPluginID(
      Settings.getNWPluginID_for_network(networkindex));
  const NetworkDriverStruct& cur_driver = ESPEasy::net::getNetworkDriverStruct(networkDriverIndex);


  // Show network settings page
  {
    html_table_class_normal();
    addFormHeader(F("Network Settings"));
    addRowLabel(F("Network Driver"));
    const nwpluginID_t choice = Settings.getNWPluginID_for_network(networkindex);

    const bool networkDriverSelectorDisabled = cur_driver.alwaysPresent && validNetworkIndex(cur_driver.fixedNetworkIndex) &&
                                               networkindex == cur_driver.fixedNetworkIndex;

    if (networkDriverSelectorDisabled) {
      addSelector_Head(F("networkDriver"));

      // Must add the fixed network driver label here.

      addSelector_Item(getNWPluginNameFromNetworkDriverIndex(networkDriverIndex),
                       choice.value,
                       true);
    } else {
      addSelector_Head_reloadOnChange(F("networkDriver"));
      addSelector_Item(F("- Standalone -"), 0, false, false, EMPTY_STRING);
      networkDriverIndex_t tmpNetworkDriverIndex{};

      while (validNetworkDriverIndex(tmpNetworkDriverIndex))
      {
        const NetworkDriverStruct& driver = getNetworkDriverStruct(tmpNetworkDriverIndex);
        const nwpluginID_t number         = getNWPluginID_from_NetworkDriverIndex(tmpNetworkDriverIndex);
        const bool disabled               = driver.alwaysPresent &&
                                            validNetworkIndex(driver.fixedNetworkIndex) &&
                                            networkindex != driver.fixedNetworkIndex;
        addSelector_Item(getNWPluginNameFromNetworkDriverIndex(tmpNetworkDriverIndex),
                         number.value,
                         choice == number,
                         disabled);
        ++tmpNetworkDriverIndex;
      }
    }
    addSelector_Foot();

  }

  # ifndef LIMIT_BUILD_SIZE
  addRTDNetworkDriverButton(getNWPluginID_from_NetworkDriverIndex(networkDriverIndex));
  # endif // ifndef LIMIT_BUILD_SIZE

  if (Settings.getNWPluginID_for_network(networkindex)) {
    // TODO TD-er: Add driver specifics from NetworkDriverStruct

    // Load network specific settings
    struct EventStruct TempEvent;
    TempEvent.NetworkIndex = networkindex;

    String str;
    do_NWPluginCall(networkDriverIndex, NWPlugin::Function::NWPLUGIN_WEBFORM_LOAD, &TempEvent, str);
# ifdef ESP32

    if (do_NWPluginCall(networkDriverIndex, NWPlugin::Function::NWPLUGIN_GET_INTERFACE, &TempEvent, str))
    {
      {
        addFormSubHeader(F("Network Interface"));

        {
          String str;
          const bool res = NWPluginCall(NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_HW_ADDRESS, &TempEvent, str);

          if (res && !TempEvent.String1.isEmpty()) {
            addRowLabel(TempEvent.String1);
          } else {
            addRowLabel(F("MAC Address"));
          }
          addHtml_pre(res
            ? str
            : TempEvent.networkInterface->macAddress());
        }

        const NWPlugin::NetforkFlags flags[] = {
          NWPlugin::NetforkFlags::DHCP_client,
          NWPlugin::NetforkFlags::DHCP_server,
          NWPlugin::NetforkFlags::AutoUp,
          NWPlugin::NetforkFlags::GratuituousArp,
          NWPlugin::NetforkFlags::EventIPmodified,
          NWPlugin::NetforkFlags::isPPP,
          NWPlugin::NetforkFlags::isBridge,
#  if CONFIG_LWIP_IPV6
          NWPlugin::NetforkFlags::MLD_v6_report,
          NWPlugin::NetforkFlags::IPv6_autoconf_enabled,
#  endif // if CONFIG_LWIP_IPV6
        };

        addRowLabel(F("Flags"));
        String labels_str;

        for (size_t i = 0; i < NR_ELEMENTS(flags); ++i) {
          if (NWPlugin::isFlagSet(flags[i], TempEvent.networkInterface)) {
            if (!labels_str.isEmpty()) {
              labels_str += F("<br>");
            }
            labels_str += NWPlugin::toString(flags[i]);

            if (flags[i] == NWPlugin::NetforkFlags::DHCP_client) {
              if (TempEvent.networkInterface->getStatusBits() & ESP_NETIF_HAS_STATIC_IP_BIT) {
                labels_str += F(" (static IP)");
              }
            }
          }
        }
        addHtml_pre(labels_str);
      }
      {
        addFormSubHeader(F("IP Config"));

        const NWPlugin::IP_type ip_types[] = {
          NWPlugin::IP_type::inet,
          NWPlugin::IP_type::network_id_cdr,
          NWPlugin::IP_type::netmask,
          NWPlugin::IP_type::broadcast,
          NWPlugin::IP_type::gateway,
          NWPlugin::IP_type::dns1,
          NWPlugin::IP_type::dns2,
#  if CONFIG_LWIP_IPV6
          NWPlugin::IP_type::ipv6_unknown,
          NWPlugin::IP_type::ipv6_global,
          NWPlugin::IP_type::ipv6_link_local,
          NWPlugin::IP_type::ipv6_site_local,
          NWPlugin::IP_type::ipv6_unique_local,
          NWPlugin::IP_type::ipv4_mapped_ipv6,
#  endif // if CONFIG_LWIP_IPV6

        };

        for (size_t i = 0; i < NR_ELEMENTS(ip_types); ++i) {

          PrintToString str;

          if (NWPlugin::print_IP_address(ip_types[i], TempEvent.networkInterface, str)) {
            addRowLabel(NWPlugin::toString(ip_types[i]));
            addHtml_pre(str.get());
          }
        }
      }
    }
# endif // ifdef ESP32

    addFormSeparator(2);

    // Separate enabled checkbox as it doesn't need to use the NetworkSettings.
    // So NetworkSettings object can be destructed before network specific settings are loaded.
    addNetworkEnabledForm(networkindex);
  }

  addFormSeparator(2);
  html_TR_TD();
  html_TD();
  addButton(F("network"), F("Close"));
  addSubmitButton();
  html_end_table();
  html_end_form();

}

#endif // ifdef WEBSERVER_NETWORK
