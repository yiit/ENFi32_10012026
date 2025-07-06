#include "ESPEasy_common.h"

#ifdef USES_NW002

// #######################################################################################################
// ########################### Network Plugin 002: WiFi Access Point #####################################
// #######################################################################################################

# define NWPLUGIN_002
# define NWPLUGIN_ID_002         2
# define NWPLUGIN_NAME_002       "WiFi AP"

# include "src/DataStructs/ESPEasy_EventStruct.h"
# include "src/Globals/NWPlugins.h"
# include "src/Helpers/ESPEasy_Storage.h"
# include "src/Helpers/_NWPlugin_init.h"

# include "src/Globals/SecuritySettings.h"
# include "src/WebServer/common.h"
# include "src/Helpers/StringConverter.h"
# include "src/WebServer/ESPEasy_WebServer.h"
# include "src/Globals/Settings.h"
# include "src/WebServer/Markup_Forms.h"
# include "src/WebServer/Markup.h"
# include "src/ESPEasyCore/ESPEasyWifi_abstracted.h"

bool NWPlugin_002(NWPlugin::Function function, struct EventStruct *event, String& string)
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
      string = F(NWPLUGIN_NAME_002);
      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_SAVE:
    {



      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_LOAD:
    {


      break;
    }

    case NWPlugin::Function::NWPLUGIN_INIT:
    {
      break;
    }


    default:
      break;

  }
  return success;
}

#endif // ifdef USES_NW002
