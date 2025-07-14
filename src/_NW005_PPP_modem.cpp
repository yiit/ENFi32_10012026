#include "ESPEasy_common.h"

#ifdef USES_NW005

// #######################################################################################################
// ########################### Network Plugin 005: PPP modem #############################################
// #######################################################################################################

# define NWPLUGIN_005
# define NWPLUGIN_ID_005         5
# define NWPLUGIN_NAME_005       "PPP modem"

#include "src/NWPluginStructs/NW005_data_struct_PPP_modem.h"

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

#include <ESPEasySerialPort.h>

#include <PPP.h>

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
      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_PORT:
    {
      break;
    }


    case NWPlugin::Function::NWPLUGIN_WEBFORM_SAVE:
    {

      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_LOAD:
    {
      // TODO TD-er: We cannot use ESPEasySerialPort here as PPPClass needs to handle the pins using periman
{
    const int ids[] = {
    static_cast<int>(ESPEasySerialPort::serial0)
#if USABLE_SOC_UART_NUM > 1
    ,static_cast<int>(ESPEasySerialPort::serial1)
#endif
#if USABLE_SOC_UART_NUM > 2
    ,static_cast<int>(ESPEasySerialPort::serial2)
#endif 
#if USABLE_SOC_UART_NUM > 3
    ,static_cast<int>(ESPEasySerialPort::serial3)
#endif 
#if USABLE_SOC_UART_NUM > 4
    ,static_cast<int>(ESPEasySerialPort::serial4)
#endif 
#if USABLE_SOC_UART_NUM > 5
    ,static_cast<int>(ESPEasySerialPort::serial5)
#endif 
  };

  constexpr int NR_ESPEASY_SERIAL_TYPES = NR_ELEMENTS(ids);
  const __FlashStringHelper* options[] = {
    serialHelper_getSerialTypeLabel(ESPEasySerialPort::serial0)
#if USABLE_SOC_UART_NUM > 1
    ,serialHelper_getSerialTypeLabel(ESPEasySerialPort::serial1)
#endif
#if USABLE_SOC_UART_NUM > 2
    ,serialHelper_getSerialTypeLabel(ESPEasySerialPort::serial2)
#endif 
#if USABLE_SOC_UART_NUM > 3
    ,serialHelper_getSerialTypeLabel(ESPEasySerialPort::serial3)
#endif 
#if USABLE_SOC_UART_NUM > 4
    ,serialHelper_getSerialTypeLabel(ESPEasySerialPort::serial4)
#endif 
#if USABLE_SOC_UART_NUM > 5
    ,serialHelper_getSerialTypeLabel(ESPEasySerialPort::serial5)
#endif 
  };
  FormSelectorOptions selector(NR_ESPEASY_SERIAL_TYPES, options, ids);

//  selector.addFormSelector(F("Serial Port"), F("serPort"), port);

}



      break;
    }

    case NWPlugin::Function::NWPLUGIN_INIT:
    {
      initNWPluginData(event->NetworkIndex, new (std::nothrow) NW005_data_struct_PPP_modem);
      NW005_data_struct_PPP_modem *NW_data = static_cast<NW005_data_struct_PPP_modem *>(getNWPluginData(event->NetworkIndex));
      if (NW_data) {
        
      }
      break;
    }

    case NWPlugin::Function::NWPLUGIN_EXIT:
    {
      break;
    }

    default:
      break;

  }
  return success;
}

#endif // ifdef USES_NW005
