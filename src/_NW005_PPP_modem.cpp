#include "ESPEasy_common.h"

#ifdef USES_NW005

// #######################################################################################################
// ########################### Network Plugin 005: PPP modem #############################################
// #######################################################################################################

# define NWPLUGIN_005
# define NWPLUGIN_ID_005         5
# define NWPLUGIN_NAME_005       "PPP modem"

# include "src/NWPluginStructs/NW005_data_struct_PPP_modem.h"

# include "src/DataStructs/ESPEasy_EventStruct.h"

# include "src/ESPEasyCore/ESPEasyNetwork.h"
# include "src/ESPEasyCore/ESPEasyWifi_abstracted.h"

# include "src/Globals/NWPlugins.h"
# include "src/Globals/SecuritySettings.h"
# include "src/Globals/Settings.h"

# include "src/Helpers/ESPEasy_Storage.h"
# include "src/Helpers/PrintToString.h"
# include "src/Helpers/StringConverter.h"
# include "src/Helpers/_NWPlugin_Helper_webform.h"
# include "src/Helpers/_NWPlugin_init.h"
# include "src/Helpers/_Plugin_Helper_serial.h"

# include "src/WebServer/ESPEasy_WebServer.h"
# include "src/WebServer/HTML_Print.h"
# include "src/WebServer/HTML_wrappers.h"
# include "src/WebServer/Markup.h"
# include "src/WebServer/Markup_Forms.h"
# include "src/WebServer/common.h"

# include <ESPEasySerialPort.h>

# include <PPP.h>

bool NWPlugin_005(NWPlugin::Function function, struct EventStruct *event, String& string)
{
  bool success = false;

  switch (function)
  {
    case NWPlugin::Function::NWPLUGIN_DRIVER_ADD:
    {
      NetworkDriverStruct& nw = getNetworkDriverStruct(networkDriverIndex_t::toNetworkDriverIndex(event->idx));
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
      event->networkInterface = &PPP;
      success                 = event->networkInterface != nullptr;
      break;
    }
# endif // ifdef ESP32


    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_CONNECTED:
    {
      success = PPP.attached();

      if (success) {
        string = strformat(
          F("%s (%s dBm)"),
          PPP.operatorName().c_str(),
          NW005_data_struct_PPP_modem::getRSSI().c_str());
      }

      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_PORT:
    {
      break;
    }


    case NWPlugin::Function::NWPLUGIN_WEBFORM_SAVE:
    {
      NW005_data_struct_PPP_modem *NW_data = static_cast<NW005_data_struct_PPP_modem *>(getNWPluginData(event->NetworkIndex));
      bool mustCleanup                     = NW_data == nullptr;

      if (mustCleanup) {
        NW_data = new (std::nothrow) NW005_data_struct_PPP_modem(event->NetworkIndex);
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
      NW005_data_struct_PPP_modem *NW_data = static_cast<NW005_data_struct_PPP_modem *>(getNWPluginData(event->NetworkIndex));
      bool mustCleanup                     = NW_data == nullptr;

      if (mustCleanup) {
        NW_data = new (std::nothrow) NW005_data_struct_PPP_modem(event->NetworkIndex);
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
      initNWPluginData(event->NetworkIndex, new (std::nothrow) NW005_data_struct_PPP_modem(event->NetworkIndex));
      NW005_data_struct_PPP_modem *NW_data = static_cast<NW005_data_struct_PPP_modem *>(getNWPluginData(event->NetworkIndex));

      if (NW_data) {
        success = NW_data->init(event);
      }
      break;
    }

    case NWPlugin::Function::NWPLUGIN_EXIT:
    {
      NW005_data_struct_PPP_modem *NW_data = static_cast<NW005_data_struct_PPP_modem *>(getNWPluginData(event->NetworkIndex));

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

#endif // ifdef USES_NW005
