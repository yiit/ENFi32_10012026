#include "../../ESPEasy_common.h"

#ifdef USES_NW004

// #######################################################################################################
// ########################### Network Plugin 004: Ethernet SPI ##########################################
// #######################################################################################################

# define NWPLUGIN_004
# define NWPLUGIN_ID_004         4
# define NWPLUGIN_NAME_004       "Ethernet (SPI)"

# include "../../src/DataStructs/ESPEasy_EventStruct.h"
# include "../net/eth/ESPEasyEth.h"
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
# include "../net/Globals/NWPlugins.h"
# include "../net/Helpers/_NWPlugin_Helper_webform.h"
# include "../net/Helpers/_NWPlugin_init.h"
# include "../net/Helpers/NW_info_writer.h"

# include "../net/NWPluginStructs/NW004_data_struct_ETH_SPI.h"


# include <pins_arduino.h>

namespace ESPEasy {
namespace net {

bool NWPlugin_004(NWPlugin::Function function, EventStruct *event, String& string)
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

    case NWPlugin::Function::NWPLUGIN_LOAD_DEFAULTS:
    {
      Settings.setRoutePrio_for_network(event->NetworkIndex, 50);
      Settings.setNetworkInterfaceSubnetBlockClientIP(event->NetworkIndex, false);
      Settings.setNetworkInterfaceStartupDelayAtBoot(event->NetworkIndex, 500 * event->NetworkIndex);

      ESPEasy_key_value_store kvs;
      ESPEasy::net::eth::NW004_data_struct_ETH_SPI::loadDefaults(
        &kvs, 
        event->NetworkIndex, 
        ESPEasy::net::nwpluginID_t(NWPLUGIN_ID_004));

      break;
    }

    case NWPlugin::Function::NWPLUGIN_GET_DEVICENAME:
    {
      string = F(NWPLUGIN_NAME_004);
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
      if (isValid(Settings.ETH_Phy_Type) && isSPI_EthernetType(Settings.ETH_Phy_Type)) {
        success = ESPEasy::net::write_Eth_HW_Address(ETH, event->kvWriter);
      }
      break;
    }
#ifndef LIMIT_BUILD_SIZE
    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_PORT:
    {
      if (event->kvWriter) {
        if (isValid(Settings.ETH_Phy_Type) && isSPI_EthernetType(Settings.ETH_Phy_Type)) {
          int8_t spi_gpios[3]{};

          if (Settings.getSPI_pins(spi_gpios)) {
            const __FlashStringHelper*labels[] = {
              F("CLK"), F("MISO"), F("MOSI"), F("CS"), F("IRQ"), F("RST") };
            const int pins[] = {
              spi_gpios[0],
              spi_gpios[1],
              spi_gpios[2],
              Settings.ETH_Pin_mdc_cs,
              Settings.ETH_Pin_mdio_irq,
              Settings.ETH_Pin_power_rst
            };

            success = write_NetworkPort(labels, pins, NR_ELEMENTS(labels), event->kvWriter);
          }
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
# if ESP_IDF_VERSION_MAJOR >= 5
#  if CONFIG_ETH_SPI_ETHERNET_DM9051
          toString(EthPhyType_t::DM9051),
#  endif // if CONFIG_ETH_SPI_ETHERNET_DM9051
#  if CONFIG_ETH_SPI_ETHERNET_W5500
          toString(EthPhyType_t::W5500),
#  endif // if CONFIG_ETH_SPI_ETHERNET_W5500
#  if CONFIG_ETH_SPI_ETHERNET_KSZ8851SNL
          toString(EthPhyType_t::KSZ8851),
#  endif // if CONFIG_ETH_SPI_ETHERNET_KSZ8851SNL
# endif // if ESP_IDF_VERSION_MAJOR >= 5
        };
        const int ethPhyTypes_index[] = {
          static_cast<int>(EthPhyType_t::notSet),

# if ESP_IDF_VERSION_MAJOR >= 5
#  if CONFIG_ETH_SPI_ETHERNET_DM9051
          static_cast<int>(EthPhyType_t::DM9051),
#  endif // if CONFIG_ETH_SPI_ETHERNET_DM9051
#  if CONFIG_ETH_SPI_ETHERNET_W5500
          static_cast<int>(EthPhyType_t::W5500),
#  endif // if CONFIG_ETH_SPI_ETHERNET_W5500
#  if CONFIG_ETH_SPI_ETHERNET_KSZ8851SNL
          static_cast<int>(EthPhyType_t::KSZ8851),
#  endif // if CONFIG_ETH_SPI_ETHERNET_KSZ8851SNL
# endif // if ESP_IDF_VERSION_MAJOR >= 5
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

      # define MDC_CS_PIN_DESCR   "Ethernet CS pin"
      # define MIO_IRQ_PIN_DESCR  "Ethernet IRQ pin"
      # define PWR_RST_PIN_DESCR  "Ethernet RST pin"

      addFormNumericBox(F("Ethernet PHY Address"), F("ethphy"), Settings.ETH_Phy_Addr, -1, 127);
      addFormNote(F("I&sup2;C-address of Ethernet PHY"));
      addFormPinSelect(PinSelectPurpose::Ethernet, formatGpioName_output(
                         F(MDC_CS_PIN_DESCR)),
                       F("ethmdc"), Settings.ETH_Pin_mdc_cs);
      addFormPinSelect(PinSelectPurpose::Ethernet, formatGpioName_input(
                         F(MIO_IRQ_PIN_DESCR)),
                       F("ethmdio"), Settings.ETH_Pin_mdio_irq);
      addFormPinSelect(PinSelectPurpose::Ethernet, formatGpioName_output(
                         F(PWR_RST_PIN_DESCR)),
                       F("ethpower"), Settings.ETH_Pin_power_rst);
      break;
    }


    case NWPlugin::Function::NWPLUGIN_INIT:
    {
      initNWPluginData(event->NetworkIndex, new (std::nothrow) ESPEasy::net::eth::NW004_data_struct_ETH_SPI(event->NetworkIndex));
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

    case NWPlugin::Function::NWPLUGIN_PRIORITY_ROUTE_CHANGED:
    {
      ESPEasy::net::eth::NW004_data_struct_ETH_SPI *NW_data =
        static_cast<ESPEasy::net::eth::NW004_data_struct_ETH_SPI *>(getNWPluginData(event->NetworkIndex));

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

#endif // ifdef USES_NW004
