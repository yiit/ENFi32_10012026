#include "../../ESPEasy_common.h"

#ifdef USES_NW005

// #######################################################################################################
// ########################### Network Plugin 005: PPP modem #############################################
// #######################################################################################################

# define NWPLUGIN_005
# define NWPLUGIN_ID_005         5
# define NWPLUGIN_NAME_005       "PPP modem"

# include "../../src/DataStructs/ESPEasy_EventStruct.h"
# include "../net/ESPEasyNetwork.h"
# include "../../src/Globals/SecuritySettings.h"
# include "../../src/Globals/Settings.h"
# include "../../src/Helpers/ESPEasy_Storage.h"
# include "../../src/Helpers/PrintToString.h"
# include "../../src/Helpers/StringConverter.h"
# include "../../src/Helpers/_Plugin_Helper_serial.h"
# include "../../src/WebServer/ESPEasy_WebServer.h"
# include "../../src/WebServer/HTML_Print.h"
# include "../../src/WebServer/HTML_wrappers.h"
# include "../../src/WebServer/Markup.h"
# include "../../src/WebServer/Markup_Forms.h"
# include "../../src/WebServer/common.h"
# include "../net/Globals/NWPlugins.h"
# include "../net/Helpers/_NWPlugin_Helper_webform.h"
# include "../net/Helpers/_NWPlugin_init.h"
# include "../net/NWPluginStructs/NW005_data_struct_PPP_modem.h"


# include <ESPEasySerialPort.h>

# include <PPP.h>

namespace ESPEasy {
namespace net {

bool NWPlugin_005(NWPlugin::Function function, EventStruct *event, String& string)
{
  bool success = false;

  switch (function)
  {
    case NWPlugin::Function::NWPLUGIN_DRIVER_ADD:
    {
      NetworkDriverStruct& nw = getNetworkDriverStruct(networkDriverIndex_t::toNetworkDriverIndex(event->idx));
      nw.onlySingleInstance = true;
      nw.alwaysPresent      = false;
      break;
    }

    case NWPlugin::Function::NWPLUGIN_GET_DEVICENAME:
    {
      string = F(NWPLUGIN_NAME_005);
      break;
    }

# ifdef ESP32
    case NWPlugin::Function::NWPLUGIN_GET_INTERFACE:
    {
      ESPEasy::net::ppp::NW005_data_struct_PPP_modem *NW_data =
        static_cast<ESPEasy::net::ppp::NW005_data_struct_PPP_modem *>(getNWPluginData(event->NetworkIndex));

      if (NW_data && NW_data->attached()) {
        event->networkInterface = &PPP;
        success                 = event->networkInterface != nullptr;
      }
      break;
    }
# endif // ifdef ESP32


    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_CONNECTED:
    {
      ESPEasy::net::ppp::NW005_data_struct_PPP_modem *NW_data =
        static_cast<ESPEasy::net::ppp::NW005_data_struct_PPP_modem *>(getNWPluginData(event->NetworkIndex));

      if (NW_data) {
        success = NW_data->attached();

        if (success) {
          string = strformat(
            F("%s (%s dBm)"),
            NW_data->operatorName().c_str(),
            NW_data->getRSSI().c_str());
        }
      }

      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_HW_ADDRESS:
    {
      ESPEasy::net::ppp::NW005_data_struct_PPP_modem *NW_data =
        static_cast<ESPEasy::net::ppp::NW005_data_struct_PPP_modem *>(getNWPluginData(event->NetworkIndex));

      if (NW_data) {
        string         = NW_data->IMEI();
        event->String1 = F("IMEI");
      }

      // Still mark success = true to prevent a call to get the MAC address
      success = true;
      break;
    }


    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_PORT:
    {
      ESPEasy::net::ppp::NW005_data_struct_PPP_modem *NW_data =
        static_cast<ESPEasy::net::ppp::NW005_data_struct_PPP_modem *>(getNWPluginData(event->NetworkIndex));

      if (NW_data) {
        success = NW_data->webform_getPort(string);
      }
      break;
    }


    case NWPlugin::Function::NWPLUGIN_WEBFORM_SAVE:
    {
      ESPEasy::net::ppp::NW005_data_struct_PPP_modem *NW_data =
        static_cast<ESPEasy::net::ppp::NW005_data_struct_PPP_modem *>(getNWPluginData(event->NetworkIndex));
      bool mustCleanup = NW_data == nullptr;

      if (mustCleanup) {
        NW_data = new (std::nothrow) ESPEasy::net::ppp::NW005_data_struct_PPP_modem(event->NetworkIndex);
        NW_data->init_KVS();
      }

      if (NW_data) {
        NW_data->webform_save(event);

        if (mustCleanup) { delete NW_data; }
        success = true;
      }

      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_LOAD:
    {
      ESPEasy::net::ppp::NW005_data_struct_PPP_modem *NW_data =
        static_cast<ESPEasy::net::ppp::NW005_data_struct_PPP_modem *>(getNWPluginData(event->NetworkIndex));
      bool mustCleanup = NW_data == nullptr;

      if (mustCleanup) {
        NW_data = new (std::nothrow) ESPEasy::net::ppp::NW005_data_struct_PPP_modem(event->NetworkIndex);
        NW_data->init_KVS();
      }

      if (NW_data) {
        NW_data->webform_load(event);
        success = true;

        if (mustCleanup) { delete NW_data; }

      }

      break;
    }

    case NWPlugin::Function::NWPLUGIN_INIT:
    {
      initNWPluginData(event->NetworkIndex, new (std::nothrow) ESPEasy::net::ppp::NW005_data_struct_PPP_modem(event->NetworkIndex));
      ESPEasy::net::ppp::NW005_data_struct_PPP_modem *NW_data =
        static_cast<ESPEasy::net::ppp::NW005_data_struct_PPP_modem *>(getNWPluginData(event->NetworkIndex));

      if (NW_data) {
        success = NW_data->init(event);
      }
      break;
    }

    case NWPlugin::Function::NWPLUGIN_EXIT:
    {
      ESPEasy::net::ppp::NW005_data_struct_PPP_modem *NW_data =
        static_cast<ESPEasy::net::ppp::NW005_data_struct_PPP_modem *>(getNWPluginData(event->NetworkIndex));

      if (NW_data) {
        success = NW_data->exit(event);
      }
      break;
    }

    default:
      break;

  }
  return success;
}

} // namespace net
} // namespace ESPEasy

#endif // ifdef USES_NW005
