#include "../WebServer/NetworkPage.h"

#ifdef WEBSERVER_NETWORK

# include "../WebServer/ESPEasy_WebServer.h"
# include "../WebServer/HTML_wrappers.h"
# include "../WebServer/Markup.h"
# include "../WebServer/Markup_Buttons.h"
# include "../WebServer/Markup_Forms.h"

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

  const int protocol_webarg_value = getFormItemInt(F("protocol"), -1);

  // submitted data
  if ((protocol_webarg_value != -1) && !networkNotSet)
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
    //handle_networks_NetworkSettingsPage(networkindex);
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

void handle_networks_CopySubmittedSettings_CPluginCall(uint8_t networkindex)
{}

void handle_networks_ShowAllNetworksTable()
{}


void handle_networks_NetworkSettingsPage(networkIndex_t networkindex)
{}



#endif // ifdef WEBSERVER_NETWORK
