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
      /*
       # if CONFIG_IDF_TARGET_ESP32P4
         Settings.ETH_Clock_Mode    = EthClockMode_t::Ext_crystal;
         Settings.ETH_Phy_Type      = EthPhyType_t::TLK110;
         Settings.ETH_Phy_Addr      = ETH_PHY_ADDR;
         Settings.ETH_Pin_mdc_cs    = ETH_PHY_MDC;
         Settings.ETH_Pin_mdio_irq  = ETH_PHY_MDIO;
         Settings.ETH_Pin_power_rst = ETH_PHY_POWER;
       # else // if CONFIG_IDF_TARGET_ESP32P4
         Settings.ETH_Clock_Mode = EthClockMode_t::Ext_crystal_osc;
       # endif // if CONFIG_IDF_TARGET_ESP32P4
       */
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


    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_CONNECTED:
    {
      success = ETH.connected();

      if (ETH.linkUp()) {
        string  = ETH.linkSpeed();
        string += ETH.fullDuplex() ? F("Mbps FD") : F("Mbps HD");

        if (!ETH.autoNegotiation()) { string += F("(manual)"); }
      }
      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_PORT:
    {
      break;
    }

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
# if CONFIG_ETH_USE_ESP32_EMAC && FEATURE_ETHERNET
      Settings.ETH_Clock_Mode = static_cast<EthClockMode_t>(getFormItemInt(F("ethclock")));
# endif
      Settings.NetworkMedium = static_cast<NetworkMedium_t>(getFormItemInt(F("ethwifi")));

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

# if CONFIG_ETH_USE_ESP32_EMAC && FEATURE_ETHERNET
          toString(EthPhyType_t::LAN8720),
          toString(EthPhyType_t::TLK110),
          toString(EthPhyType_t::RTL8201),
#  if ETH_TYPE_JL1101_SUPPORTED
          toString(EthPhyType_t::JL1101),
#  endif
          toString(EthPhyType_t::DP83848),
          toString(EthPhyType_t::KSZ8041),
          toString(EthPhyType_t::KSZ8081),
# endif // if CONFIG_ETH_USE_ESP32_EMAC && FEATURE_ETHERNET

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

# if CONFIG_ETH_USE_ESP32_EMAC && FEATURE_ETHERNET
          static_cast<int>(EthPhyType_t::LAN8720),
          static_cast<int>(EthPhyType_t::TLK110),
          static_cast<int>(EthPhyType_t::RTL8201),
#  if ETH_TYPE_JL1101_SUPPORTED
          static_cast<int>(EthPhyType_t::JL1101),
#  endif
          static_cast<int>(EthPhyType_t::DP83848),
          static_cast<int>(EthPhyType_t::KSZ8041),
          static_cast<int>(EthPhyType_t::KSZ8081),
# endif // if CONFIG_ETH_USE_ESP32_EMAC && FEATURE_ETHERNET

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

# if CONFIG_ETH_USE_SPI_ETHERNET && CONFIG_ETH_USE_ESP32_EMAC
#  define MDC_CS_PIN_DESCR   "Ethernet MDC/CS pin"
#  define MIO_IRQ_PIN_DESCR  "Ethernet MDIO/IRQ pin"
#  define PWR_RST_PIN_DESCR  "Ethernet Power/RST pin"
# elif CONFIG_ETH_USE_SPI_ETHERNET
#  define MDC_CS_PIN_DESCR   "Ethernet CS pin"
#  define MIO_IRQ_PIN_DESCR  "Ethernet IRQ pin"
#  define PWR_RST_PIN_DESCR  "Ethernet RST pin"
# else // #elif CONFIG_ETH_USE_ESP32_EMAC && FEATURE_ETHERNET
#  define MDC_CS_PIN_DESCR   "Ethernet MDC pin"
#  define MIO_IRQ_PIN_DESCR  "Ethernet MIO pin"
#  define PWR_RST_PIN_DESCR  "Ethernet Power pin"
# endif // if CONFIG_ETH_USE_SPI_ETHERNET && CONFIG_ETH_USE_ESP32_EMAC

      addFormNumericBox(F("Ethernet PHY Address"), F("ethphy"), Settings.ETH_Phy_Addr, -1, 127);
      addFormNote(F("I&sup2;C-address of Ethernet PHY"
# if CONFIG_ETH_USE_ESP32_EMAC && FEATURE_ETHERNET
                    " (0 or 1 for LAN8720, 31 for TLK110, -1 autodetect)"
# endif
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
# if CONFIG_ETH_USE_ESP32_EMAC && FEATURE_ETHERNET
      addRowLabel_tr_id(F("Ethernet Clock"), F("ethclock"));
      {
        #  if CONFIG_IDF_TARGET_ESP32
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
        #  endif // if CONFIG_IDF_TARGET_ESP32
        #  if CONFIG_IDF_TARGET_ESP32P4
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
        #  endif // if CONFIG_IDF_TARGET_ESP32P4
        const FormSelectorOptions selector(
          NR_ELEMENTS(ethClockOptions),
          ethClockOptions,
          indices);
        selector.addSelector(F("ethclock"), static_cast<int>(Settings.ETH_Clock_Mode));
      }
# endif // if CONFIG_ETH_USE_ESP32_EMAC && FEATURE_ETHERNET

      break;
    }

    case NWPlugin::Function::NWPLUGIN_INIT:
    {
      ESPEasy::net::eth::ETHConnectRelaxed();
      break;
    }

    case NWPlugin::Function::NWPLUGIN_EXIT:
    {
      ETH.end();
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
