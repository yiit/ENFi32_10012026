#include "../../ESPEasy_common.h"

#ifdef USES_NW003

// #######################################################################################################
// ########################### Network Plugin 003: Ethernet RMII #########################################
// #######################################################################################################

# define NWPLUGIN_003
# define NWPLUGIN_ID_003         3
# define NWPLUGIN_NAME_003       "Ethernet (RMII)"

# include "../../src/DataStructs/ESPEasy_EventStruct.h"
# include "../../src/Globals/SecuritySettings.h"
# include "../../src/Globals/Settings.h"
# include "../../src/Helpers/ESPEasy_Storage.h"
# include "../../src/Helpers/PrintToString.h"
# include "../../src/Helpers/StringConverter.h"
# include "../../src/WebServer/ESPEasy_WebServer.h"
# include "../../src/WebServer/HTML_Print.h"
# include "../../src/WebServer/HTML_wrappers.h"
# include "../../src/WebServer/Markup.h"
# include "../../src/WebServer/Markup_Forms.h"
# include "../../src/WebServer/common.h"
# include "../net/eth/ESPEasyEth.h"
# include "../net/Globals/NWPlugins.h"
# include "../net/Helpers/_NWPlugin_Helper_webform.h"
# include "../net/Helpers/_NWPlugin_init.h"
# include "../net/Helpers/NW_info_writer.h"

# include "../net/NWPluginStructs/NW003_data_struct_ETH_RMII.h"

# include <pins_arduino.h>

namespace ESPEasy {
namespace net {

bool NWPlugin_003(NWPlugin::Function function, EventStruct *event, String& string)
{
  bool success = false;

  switch (function)
  {
    case NWPlugin::Function::NWPLUGIN_DRIVER_ADD:
    {
      NetworkDriverStruct& nw = getNetworkDriverStruct(networkDriverIndex_t::toNetworkDriverIndex(event->idx));
      nw.onlySingleInstance = true;
# ifdef ESP32P4
      nw.alwaysPresent         = true;
      nw.enabledOnFactoryReset = true;
      nw.fixedNetworkIndex     = NWPLUGIN_ID_003 - 1; // Start counting at 0
# else // ifdef ESP32P4
      nw.alwaysPresent = false;
# endif // ifdef ESP32P4
      break;
    }

    case NWPlugin::Function::NWPLUGIN_LOAD_DEFAULTS:
    {
      Settings.setRoutePrio_for_network(event->NetworkIndex, 50);
      Settings.setNetworkInterfaceSubnetBlockClientIP(event->NetworkIndex, false);
      Settings.setNetworkInterfaceStartupDelayAtBoot(event->NetworkIndex, 500 * event->NetworkIndex);

      ESPEasy_key_value_store kvs;
      ESPEasy::net::eth::NW003_data_struct_ETH_RMII::loadDefaults(
        &kvs, 
        event->NetworkIndex, 
        ESPEasy::net::nwpluginID_t(NWPLUGIN_ID_003));

      break;
    }

    case NWPlugin::Function::NWPLUGIN_GET_DEVICENAME:
    {
      string = F(NWPLUGIN_NAME_003);
      break;
    }

    # ifdef ESP32
    case NWPlugin::Function::NWPLUGIN_GET_INTERFACE:
    {
      event->networkInterface = &ETH;
      success                 = event->networkInterface != nullptr;
      break;
    }
# endif // ifdef ESP32

    case NWPlugin::Function::NWPLUGIN_WEBSERVER_SHOULD_RUN:
    {
      success = ETH.connected();
      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_CONNECTED:
    {
      success = write_Eth_Show_Connected(ETH, event->kvWriter);
      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_HW_ADDRESS:
    {

      if (isValid(Settings.ETH_Phy_Type) && !isSPI_EthernetType(Settings.ETH_Phy_Type)) {
        success = ESPEasy::net::write_Eth_HW_Address(ETH, event->kvWriter);
      }
      break;
    }
#ifndef LIMIT_BUILD_SIZE
    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_PORT:
    {
      if (event->kvWriter) {
        if (isValid(Settings.ETH_Phy_Type) && !isSPI_EthernetType(Settings.ETH_Phy_Type)) {

          const __FlashStringHelper*labels[] = {
            F("MDC"), F("MDIO"), F("Power") };
          const int pins[] = {
            Settings.ETH_Pin_mdc_cs,
            Settings.ETH_Pin_mdio_irq,
            Settings.ETH_Pin_power_rst
          };
          success = write_NetworkPort(labels, pins, NR_ELEMENTS(labels), event->kvWriter);
        }
      }
      break;
    }
#endif

    case NWPlugin::Function::NWPLUGIN_WEBFORM_SAVE:
    {

      webArg2ip(F("espethip"),      Settings.ETH_IP);
      webArg2ip(F("espethgateway"), Settings.ETH_Gateway);
      webArg2ip(F("espethsubnet"),  Settings.ETH_Subnet);
      webArg2ip(F("espethdns"),     Settings.ETH_DNS);

      Settings.ETH_Phy_Addr      = getFormItemInt(F("ethphy"));
      Settings.ETH_Pin_mdc_cs    = getFormItemInt(F("ethmdc"));
      Settings.ETH_Pin_mdio_irq  = getFormItemInt(F("ethmdio"));
      Settings.ETH_Pin_power_rst = getFormItemInt(F("ethpower"));
      Settings.ETH_Phy_Type      = static_cast<EthPhyType_t>(getFormItemInt(F("ethtype")));
      Settings.ETH_Clock_Mode    = static_cast<EthClockMode_t>(getFormItemInt(F("ethclock")));
      Settings.NetworkMedium     = static_cast<NetworkMedium_t>(getFormItemInt(F("ethwifi")));

      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_LOAD:
    {
      addFormSubHeader(F("Ethernet IP Settings"));

      addFormIPBox(F("ESP Ethernet IP"),         F("espethip"),      Settings.ETH_IP);
      addFormIPBox(F("ESP Ethernet Gateway"),    F("espethgateway"), Settings.ETH_Gateway);
      addFormIPBox(F("ESP Ethernet Subnetmask"), F("espethsubnet"),  Settings.ETH_Subnet);
      addFormIPBox(F("ESP Ethernet DNS"),        F("espethdns"),     Settings.ETH_DNS);
      addFormNote(F("Leave empty for DHCP"));


      addFormSubHeader(F("Ethernet"));
      addRowLabel_tr_id(F("Preferred network medium"), F("ethwifi"));
      {
        const __FlashStringHelper *ethWifiOptions[] = {
          toString(NetworkMedium_t::WIFI),
          toString(NetworkMedium_t::Ethernet)
        };
        const FormSelectorOptions  selector(NR_ELEMENTS(ethWifiOptions), ethWifiOptions);
        selector.addSelector(
          F("ethwifi"),
          static_cast<int>(Settings.NetworkMedium));
      }
      addFormNote(F("Change Switch between WiFi and Ethernet requires reboot to activate"));
      {
        const __FlashStringHelper *ethPhyTypes[] = {
          toString(EthPhyType_t::notSet),

          toString(EthPhyType_t::LAN8720),
# if ETH_PHY_LAN867X_SUPPORTED
          toString(ESPEasy::net::EthPhyType_t::LAN867X),
# endif
          toString(EthPhyType_t::TLK110),
          toString(EthPhyType_t::RTL8201),
# if ETH_TYPE_JL1101_SUPPORTED
          toString(EthPhyType_t::JL1101),
# endif
          toString(EthPhyType_t::DP83848),
          toString(EthPhyType_t::KSZ8041),
          toString(EthPhyType_t::KSZ8081),
        };
        const int ethPhyTypes_index[] = {
          static_cast<int>(EthPhyType_t::notSet),

          static_cast<int>(EthPhyType_t::LAN8720),
# if ETH_PHY_LAN867X_SUPPORTED
          static_cast<int>(ESPEasy::net::EthPhyType_t::LAN867X),
# endif
          static_cast<int>(EthPhyType_t::TLK110),
          static_cast<int>(EthPhyType_t::RTL8201),
# if ETH_TYPE_JL1101_SUPPORTED
          static_cast<int>(EthPhyType_t::JL1101),
# endif
          static_cast<int>(EthPhyType_t::DP83848),
          static_cast<int>(EthPhyType_t::KSZ8041),
          static_cast<int>(EthPhyType_t::KSZ8081),

        };

        constexpr unsigned nrItems = NR_ELEMENTS(ethPhyTypes_index);


        const int choice =
          isValid(Settings.ETH_Phy_Type)
      ? static_cast<int>(Settings.ETH_Phy_Type)
      : static_cast<int>(EthPhyType_t::notSet);

        const FormSelectorOptions selector(
          nrItems,
          ethPhyTypes,
          ethPhyTypes_index);
        selector.addFormSelector(
          F("Ethernet PHY type"),
          F("ethtype"),
          choice);
      }

      # define MDC_CS_PIN_DESCR   "Ethernet MDC pin"
      # define MIO_IRQ_PIN_DESCR  "Ethernet MIO pin"
      # define PWR_RST_PIN_DESCR  "Ethernet Power pin"

      addFormNumericBox(F("Ethernet PHY Address"), F("ethphy"), Settings.ETH_Phy_Addr, -1, 127);
      addFormNote(F("I&sup2;C-address of Ethernet PHY"
                    " (0 or 1 for LAN8720, 31 for TLK110, -1 autodetect)"
                    ));
      addFormPinSelect(PinSelectPurpose::Ethernet, formatGpioName_output(
                         F(MDC_CS_PIN_DESCR)),
                       F("ethmdc"), Settings.ETH_Pin_mdc_cs);
      addFormPinSelect(PinSelectPurpose::Ethernet, formatGpioName_input(
                         F(MIO_IRQ_PIN_DESCR)),
                       F("ethmdio"), Settings.ETH_Pin_mdio_irq);
      addFormPinSelect(PinSelectPurpose::Ethernet, formatGpioName_output(
                         F(PWR_RST_PIN_DESCR)),
                       F("ethpower"), Settings.ETH_Pin_power_rst);
      addRowLabel_tr_id(F("Ethernet Clock"), F("ethclock"));
      {
        # if CONFIG_IDF_TARGET_ESP32
        const __FlashStringHelper *ethClockOptions[] = {
          toString(EthClockMode_t::Ext_crystal_osc),
          toString(EthClockMode_t::Int_50MHz_GPIO_0),
          toString(EthClockMode_t::Int_50MHz_GPIO_16),
          toString(EthClockMode_t::Int_50MHz_GPIO_17_inv)
        };
        const int indices[] = {
          static_cast<int>(EthClockMode_t::Ext_crystal_osc),
          static_cast<int>(EthClockMode_t::Int_50MHz_GPIO_0),
          static_cast<int>(EthClockMode_t::Int_50MHz_GPIO_16),
          static_cast<int>(EthClockMode_t::Int_50MHz_GPIO_17_inv)
        };
        # endif // if CONFIG_IDF_TARGET_ESP32
        # if CONFIG_IDF_TARGET_ESP32P4
        const __FlashStringHelper *ethClockOptions[] = {
          //          toString(EthClockMode_t::Default),
          toString(EthClockMode_t::Ext_crystal),
          toString(EthClockMode_t::Int_50MHz)
        };
        const int indices[] = {
          //          static_cast<int>(EthClockMode_t::Default),
          static_cast<int>(EthClockMode_t::Ext_crystal),
          static_cast<int>(EthClockMode_t::Int_50MHz)
        };
        # endif // if CONFIG_IDF_TARGET_ESP32P4
        const FormSelectorOptions selector(
          NR_ELEMENTS(ethClockOptions),
          ethClockOptions,
          indices);
        selector.addSelector(F("ethclock"), static_cast<int>(Settings.ETH_Clock_Mode));
      }

      break;
    }

    case NWPlugin::Function::NWPLUGIN_INIT:
    {
      initNWPluginData(event->NetworkIndex, new (std::nothrow) ESPEasy::net::eth::NW003_data_struct_ETH_RMII(event->NetworkIndex));
      auto *NW_data = getNWPluginData(event->NetworkIndex);

      if (NW_data) {
        success = NW_data->init(event);
      }
      break;
    }

    case NWPlugin::Function::NWPLUGIN_EXIT:
    {
      auto *NW_data = getNWPluginData(event->NetworkIndex);

      if (NW_data) {
        NW_data->exit(event);
      }
      success = true;
      break;
    }

    case NWPlugin::Function::NWPLUGIN_TEN_PER_SECOND:
    // FIXME TD-er: Must make this act on DNS updates from other interfaces
    // Fall through
    case NWPlugin::Function::NWPLUGIN_PRIORITY_ROUTE_CHANGED:
    {
      ESPEasy::net::eth::NW003_data_struct_ETH_RMII *NW_data =
        static_cast<ESPEasy::net::eth::NW003_data_struct_ETH_RMII *>(getNWPluginData(event->NetworkIndex));

      if (NW_data) {
        success = NW_data->handle_priority_route_changed();
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


#endif // ifdef USES_NW003
