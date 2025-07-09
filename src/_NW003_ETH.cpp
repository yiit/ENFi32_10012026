#include "ESPEasy_common.h"

#ifdef USES_NW003

// #######################################################################################################
// ########################### Network Plugin 003: Ethernet ##############################################
// #######################################################################################################

# define NWPLUGIN_003
# define NWPLUGIN_ID_003         3
# define NWPLUGIN_NAME_003       "Ethernet"

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

bool NWPlugin_003(NWPlugin::Function function, struct EventStruct *event, String& string)
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
      string = F(NWPLUGIN_NAME_003);
      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_CONNECTED:
    {
      success = ETH.connected();

      if (ETH.linkUp()) {
        string = concat(ETH.linkSpeed(), ETH.fullDuplex() ? F("M FD") : F("M HD"));

        if (!ETH.autoNegotiation()) { string += F("(man)"); }
      }
      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_HOSTNAME:
    {
      string = ETH.getHostname();
      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_MAC:
    {
      string = ETH.macAddress();
      break;
    }

    case NWPlugin::Function::NWPLUGIN_WEBFORM_SHOW_IP:
    {
      string = ETH.localIP().toString();
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
# if CONFIG_ETH_USE_ESP32_EMAC
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
        selector.addSelector(F("ethwifi"), static_cast<int>(Settings.NetworkMedium));
      }
      addFormNote(F("Change Switch between WiFi and Ethernet requires reboot to activate"));
      {
        const __FlashStringHelper *ethPhyTypes[] = {
          toString(EthPhyType_t::notSet),

# if CONFIG_ETH_USE_ESP32_EMAC
          toString(EthPhyType_t::LAN8720),
          toString(EthPhyType_t::TLK110),
          toString(EthPhyType_t::RTL8201),
#  if ETH_TYPE_JL1101_SUPPORTED
          toString(EthPhyType_t::JL1101),
#  endif
          toString(EthPhyType_t::DP83848),
          toString(EthPhyType_t::KSZ8041),
          toString(EthPhyType_t::KSZ8081),
# endif // if CONFIG_ETH_USE_ESP32_EMAC

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

# if CONFIG_ETH_USE_ESP32_EMAC
          static_cast<int>(EthPhyType_t::LAN8720),
          static_cast<int>(EthPhyType_t::TLK110),
          static_cast<int>(EthPhyType_t::RTL8201),
#  if ETH_TYPE_JL1101_SUPPORTED
          static_cast<int>(EthPhyType_t::JL1101),
#  endif
          static_cast<int>(EthPhyType_t::DP83848),
          static_cast<int>(EthPhyType_t::KSZ8041),
          static_cast<int>(EthPhyType_t::KSZ8081),
# endif // if CONFIG_ETH_USE_ESP32_EMAC

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
# else // #elif CONFIG_ETH_USE_ESP32_EMAC
#  define MDC_CS_PIN_DESCR   "Ethernet MDC pin"
#  define MIO_IRQ_PIN_DESCR  "Ethernet MIO pin"
#  define PWR_RST_PIN_DESCR  "Ethernet Power pin"
# endif // if CONFIG_ETH_USE_SPI_ETHERNET && CONFIG_ETH_USE_ESP32_EMAC

      addFormNumericBox(F("Ethernet PHY Address"), F("ethphy"), Settings.ETH_Phy_Addr, -1, 127);
      addFormNote(F("I&sup2;C-address of Ethernet PHY"
# if CONFIG_ETH_USE_ESP32_EMAC
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
# if CONFIG_ETH_USE_ESP32_EMAC
      addRowLabel_tr_id(F("Ethernet Clock"), F("ethclock"));
      {
        const __FlashStringHelper *ethClockOptions[] = {
          toString(EthClockMode_t::Ext_crystal_osc),
          toString(EthClockMode_t::Int_50MHz_GPIO_0),
          toString(EthClockMode_t::Int_50MHz_GPIO_16),
          toString(EthClockMode_t::Int_50MHz_GPIO_17_inv)
        };
        const FormSelectorOptions  selector(NR_ELEMENTS(ethClockOptions), ethClockOptions);
        selector.addSelector(F("ethclock"), static_cast<int>(Settings.ETH_Clock_Mode));
      }
# endif // if CONFIG_ETH_USE_ESP32_EMAC
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

#endif // ifdef USES_NW003
