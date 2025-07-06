#include "../WebServer/NetworkPage.h"

#ifdef WEBSERVER_NETWORK

# include "../WebServer/ESPEasy_WebServer.h"
# include "../WebServer/HTML_wrappers.h"
# include "../WebServer/Markup.h"
# include "../WebServer/Markup_Buttons.h"
# include "../WebServer/Markup_Forms.h"

# include "../DataStructs/ESPEasy_EventStruct.h"
# include "../Globals/NWPlugins.h"
# include "../Globals/Settings.h"

# include "../Helpers/_NWPlugin_init.h"
# include "../Helpers/ESPEasy_Storage.h"
# include "../Helpers/StringConverter.h"
# include "../Helpers/ESPEasy_Storage.h"

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
    // TODO TD-er: Implement saving submitted settings


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

/*
   void handle_networks_clearLoadDefaults(uint8_t networkindex, NetworkSettingsStruct& NetworkSettings)
   {}
 */

/*
   void handle_networks_CopySubmittedSettings(uint8_t networkindex, NetworkSettingsStruct& NetworkSettings)
   {}
 */
void handle_networks_CopySubmittedSettings_NWPluginCall(uint8_t networkindex) {}

void handle_networks_ShowAllNetworksTable()
{
  html_table_class_multirow();
  html_TR();
  html_table_header(F(""),          70);
  html_table_header(F("Nr"),        50);
  html_table_header(F("Order"),     50);
  html_table_header(F("Enabled"),   100);
  html_table_header(F("Connected"), 100);
  html_table_header(F("Network Adapter"));
  html_table_header(F("MAC"));
  html_table_header(F("IP"));
  html_table_header(F("Port"));

  for (networkIndex_t x = 0; x < NETWORK_MAX; x++)
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
    html_TD();

    if (nwplugin_set)
    {
      addEnabled(Settings.getNetworkEnabled(x));

      html_TD();
      addHtml(getNWPluginNameFromNWPluginID(Settings.getNWPluginID_for_network(x)));
      html_TD();
      const networkDriverIndex_t NetworkDriverIndex = getNetworkDriverIndex_from_NetworkIndex(x);
      {
        String hostDescription;
        NWPluginCall(NetworkDriverIndex, NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_HOST_CONFIG, 0, hostDescription);

        if (!hostDescription.isEmpty()) {
          addHtml(hostDescription);
        }
      }

      html_TD();

      /*
         const NetworkDriverStruct& adapter = getNetworkDriverStruct(NetworkDriverIndex);

         if ((INVALID_NETWORKDRIVER_INDEX == NetworkDriverIndex) || adapter.usesPort) {
         addHtmlInt(13 == nwpluginID ? Settings.UDPPort : NetworkSettings->Port); // P2P/C013 exception
         }
       */
    }
    else {
      html_TD(3);
    }
    html_TD(3);

  }

  html_end_table();
  html_end_form();
}

void handle_networks_NetworkSettingsPage(networkIndex_t networkindex) {
  if (!validNetworkIndex(networkindex)) { return; }

  // Show network settings page
  html_table_class_normal();
  addFormHeader(F("Network Settings"));
  addRowLabel(F("Network Driver"));
  uint8_t choice = Settings.getNWPluginID_for_network(networkindex);

  addSelector_Head_reloadOnChange(F("networkDriver"));
  addSelector_Item(F("- Standalone -"), 0, false, false, EMPTY_STRING);

  networkDriverIndex_t networkDriverIndex{};

  while (validNetworkDriverIndex(networkDriverIndex))
  {
    const nwpluginID_t number = getNWPluginID_from_NetworkDriverIndex(networkDriverIndex);
    boolean disabled          = false; // !((networkindex == 0) || !NetworkDriver[x].usesMQTT);
    addSelector_Item(getNWPluginNameFromNetworkDriverIndex(networkDriverIndex),
                     number,
                     choice == number,
                     disabled);
    ++networkDriverIndex;
  }
  addSelector_Foot(true);


}

#endif // ifdef WEBSERVER_NETWORK
