#include "../NWPluginStructs/NW004_data_struct_ETH_SPI.h"

#ifdef USES_NW004

# include "../../../src/Globals/Settings.h"

# include "../../../src/Helpers/ESPEasy_time_calc.h"
# include "../../../src/Helpers/LongTermOnOffTimer.h"
# include "../../../src/Helpers/StringConverter.h"

# include "../../../src/WebServer/Markup.h"
# include "../../../src/WebServer/Markup_Forms.h"
# include "../../../src/WebServer/ESPEasy_key_value_store_webform.h"

# include "../Globals/NetworkState.h"

# include "../eth/ESPEasyEth.h"

# define NW_PLUGIN_ID  4
# define NW_PLUGIN_INTERFACE   ETH

namespace ESPEasy {
namespace net {
namespace eth {

static NWPluginData_static_runtime stats_and_cache(&NW_PLUGIN_INTERFACE);

// Keys as used in the Key-value-store
# define NW004_KEY_ETH_INDEX                         1
# define NW004_KEY_ETH_PHY_TYPE                      2
# define NW004_KEY_ETH_PHY_ADDR                      3
# define NW004_KEY_ETH_PIN_CS                        4
# define NW004_KEY_ETH_PIN_IRQ                       5
# define NW004_KEY_ETH_PIN_RST                       6
# define NW004_KEY_IP                                7
# define NW004_KEY_GW                                8
# define NW004_KEY_SN                                9
# define NW004_KEY_DNS                               10

# define NW004_MAX_KEY                               11

const __FlashStringHelper * NW004_data_struct_ETH_SPI::getLabelString(uint32_t key, bool displayString, KVS_StorageType::Enum& storageType)
{
  storageType = KVS_StorageType::Enum::int8_type;

  switch (key)
  {
    case NW004_KEY_ETH_INDEX:  return F("Index");
    case NW004_KEY_ETH_PHY_TYPE: return displayString ? F("Ethernet PHY type") : F("phytype");
    case NW004_KEY_ETH_PHY_ADDR: return displayString ? F("Ethernet PHY Address") : F("phyaddr");
    case NW004_KEY_ETH_PIN_CS: return displayString ? F("Ethernet CS pin") : F("CS");
    case NW004_KEY_ETH_PIN_IRQ: return displayString ? F("Ethernet IRQ pin") : F("IRQ");
    case NW004_KEY_ETH_PIN_RST: return displayString ? F("Ethernet RST pin") : F("RST");
    case NW004_KEY_IP:
      storageType = KVS_StorageType::Enum::string_type;
      return F("IP");
    case NW004_KEY_GW:
      storageType = KVS_StorageType::Enum::string_type;
      return displayString ? F("Gateway") : F("gw");
    case NW004_KEY_SN:
      storageType = KVS_StorageType::Enum::string_type;
      return displayString ? F("Subnetmask") : F("sn");
    case NW004_KEY_DNS:
      storageType = KVS_StorageType::Enum::string_type;
      return F("DNS");
  }
  return F("");
}

int32_t NW004_data_struct_ETH_SPI::getNextKey(int32_t key)
{
  if (key == -1) { return NW004_KEY_ETH_INDEX; }

  if (key >= NW004_MAX_KEY) { return -2; }
  return key + 1;
}

String NW004_data_struct_ETH_SPI::formatGpioLabel(uint32_t          key,
                                                  PinSelectPurpose& purpose,
                                                  bool              shortNotation)
{
  String label;

  if ((key >= NW004_KEY_ETH_PIN_CS) && (key <= NW004_KEY_ETH_PIN_RST))
  {
    KVS_StorageType::Enum storageType;
    purpose = PinSelectPurpose::Generic;
    label   = getLabelString(key, true, storageType);

    const bool optional = !shortNotation;

    switch (key)
    {
      case NW004_KEY_ETH_PIN_IRQ:
        purpose = PinSelectPurpose::Generic_input;
        label   = formatGpioName(
          label, gpio_direction::gpio_input, optional);
        break;
      case NW004_KEY_ETH_PIN_CS:
      case NW004_KEY_ETH_PIN_RST:
        purpose = PinSelectPurpose::Generic_output;
        label   = formatGpioName(
          label, gpio_direction::gpio_output, optional);
        break;
    }
  }
  return label;
}

WebFormItemParams NW004_makeWebFormItemParams(uint32_t key) {
  KVS_StorageType::Enum storageType;
  const __FlashStringHelper*label = NW004_data_struct_ETH_SPI::getLabelString(key, true, storageType);
  const __FlashStringHelper*id    = NW004_data_struct_ETH_SPI::getLabelString(key, false, storageType);

  return WebFormItemParams(label, id, storageType, key);
}

NW004_data_struct_ETH_SPI::NW004_data_struct_ETH_SPI(networkIndex_t networkIndex)
  : NWPluginData_base(nwpluginID_t(NW_PLUGIN_ID), networkIndex, &NW_PLUGIN_INTERFACE)
{
  stats_and_cache.clear(networkIndex);
  nw_event_id = Network.onEvent(NW004_data_struct_ETH_SPI::onEvent);
}

NW004_data_struct_ETH_SPI::~NW004_data_struct_ETH_SPI()
{
  if (nw_event_id != 0) {
    Network.removeEvent(nw_event_id);
  }
  nw_event_id = 0;
  stats_and_cache.processEvent_and_clear();
}

void NW004_data_struct_ETH_SPI::loadDefaults(ESPEasy_key_value_store     *kvs,
                                             ESPEasy::net::networkIndex_t networkIndex,
                                             ESPEasy::net::nwpluginID_t   nwPluginID)
{
  // Only load the defaults when the settings are empty
  if (kvs && kvs->isEmpty()) {
    if (isValid(Settings.ETH_Phy_Type) &&
        ESPEasy::net::isSPI_EthernetType(Settings.ETH_Phy_Type))
    {
      kvs->setValue(NW004_KEY_ETH_INDEX,    static_cast<int8_t>(-1));
      kvs->setValue(NW004_KEY_ETH_PHY_TYPE, static_cast<int8_t>(Settings.ETH_Phy_Type));
      kvs->setValue(NW004_KEY_ETH_PHY_ADDR, static_cast<int8_t>(Settings.ETH_Phy_Addr));
      kvs->setValue(NW004_KEY_ETH_PIN_CS,   static_cast<int8_t>(Settings.ETH_Pin_mdc_cs));
      kvs->setValue(NW004_KEY_ETH_PIN_IRQ,  static_cast<int8_t>(Settings.ETH_Pin_mdio_irq));
      kvs->setValue(NW004_KEY_ETH_PIN_RST,  static_cast<int8_t>(Settings.ETH_Pin_power_rst));

      // TODO TD-er: Copy IP info

      store_nwpluginTaskData_KVS(kvs, networkIndex, nwPluginID);
    }
    # if defined(DEFAULT_ETH_PHY_TYPE) && defined(DEFAULT_ETH_CLOCK_MODE) && defined(DEFAULT_ETH_PHY_ADDR) && \
    defined(DEFAULT_ETH_PIN_MDC) && defined(DEFAULT_ETH_PIN_MDIO)
    else if (isValid(DEFAULT_ETH_PHY_TYPE) &&
             ESPEasy::net::isSPI_EthernetType(DEFAULT_ETH_PHY_TYPE))
    {
      kvs->setValue(NW004_KEY_ETH_INDEX,    static_cast<int8_t>(-1));
      kvs->setValue(NW004_KEY_ETH_PHY_TYPE, static_cast<int8_t>(DEFAULT_ETH_PHY_TYPE));
      kvs->setValue(NW004_KEY_ETH_PHY_ADDR, static_cast<int8_t>(DEFAULT_ETH_PHY_ADDR));
      kvs->setValue(NW004_KEY_ETH_PIN_CS,   static_cast<int8_t>(DEFAULT_ETH_PIN_MDC));
      kvs->setValue(NW004_KEY_ETH_PIN_IRQ,  static_cast<int8_t>(DEFAULT_ETH_PIN_MDIO));
      kvs->setValue(NW004_KEY_ETH_PIN_RST,  static_cast<int8_t>(DEFAULT_ETH_PIN_POWER));

      // TODO TD-er: Copy IP info

      store_nwpluginTaskData_KVS(kvs, networkIndex, nwPluginID);
    }
    # endif // if defined(DEFAULT_ETH_PHY_TYPE) && defined(DEFAULT_ETH_CLOCK_MODE) && defined(DEFAULT_ETH_PHY_ADDR) &&
    // defined(DEFAULT_ETH_PIN_MDC) && defined(DEFAULT_ETH_PIN_MDIO)
  }
}

void NW004_data_struct_ETH_SPI::webform_load(EventStruct *event)
{
  addFormSubHeader(F("Ethernet IP Settings"));

  addFormIPBox(F("ESP Ethernet IP"),         F("espethip"),      Settings.ETH_IP);
  addFormIPBox(F("ESP Ethernet Gateway"),    F("espethgateway"), Settings.ETH_Gateway);
  addFormIPBox(F("ESP Ethernet Subnetmask"), F("espethsubnet"),  Settings.ETH_Subnet);
  addFormIPBox(F("ESP Ethernet DNS"),        F("espethdns"),     Settings.ETH_DNS);
  addFormNote(F("Leave empty for DHCP"));

  addFormSubHeader(F("Ethernet"));

  {
    const __FlashStringHelper *options[] = {
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
    const int ids[] = {
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

    FormSelectorOptions selector(NR_ELEMENTS(ids), options, ids);
    auto params = NW004_makeWebFormItemParams(NW004_KEY_ETH_PHY_TYPE);
    params._defaultIntValue = static_cast<int>(EthPhyType_t::notSet);
    showFormSelector(*_kvs, selector, params);
  }
  {
    auto params = NW004_makeWebFormItemParams(NW004_KEY_ETH_PHY_ADDR);
    params._min             = -1;
    params._max             = 127;
    params._defaultIntValue = 1;
    showWebformItem(*_kvs, params);
    addFormNote(F("I&sup2;C-address of Ethernet PHY"));
  }
  {
    const int gpio_keys[] = {
      NW004_KEY_ETH_PIN_CS,
      NW004_KEY_ETH_PIN_IRQ,
      NW004_KEY_ETH_PIN_RST
    };

    for (int i = 0; i < NR_ELEMENTS(gpio_keys); ++i)
    {
      const int key = gpio_keys[i];
      KVS_StorageType::Enum storageType;
      const __FlashStringHelper *id = getLabelString(key, false, storageType);
      PinSelectPurpose purpose      = PinSelectPurpose::Ethernet;
      String label                  = formatGpioLabel(key, purpose);

      int8_t pin = -1;
      _kvs->getValue(key, pin);

      addFormPinSelect(purpose, label, id, pin);
    }
  }
}

void NW004_data_struct_ETH_SPI::webform_save(EventStruct *event)        {
  // TODO TD-er: Move this to a central function, like done with import/export
  int32_t key = getNextKey(-1);

  while (key >= 0) {
    KVS_StorageType::Enum storageType;
    const __FlashStringHelper *id = getLabelString(key, false, storageType);
    storeWebformItem(*_kvs, key, storageType, id);
    key = getNextKey(key);
  }
  _store();
}

bool NW004_data_struct_ETH_SPI::webform_getPort(KeyValueWriter *writer) { return true; }

bool NW004_data_struct_ETH_SPI::init(EventStruct *event)
{
  auto data = getNWPluginData_static_runtime();

  if (data) {
    ESPEasy::net::eth::ETHConnectRelaxed(*data);
  }

  return true;
}

bool NW004_data_struct_ETH_SPI::exit(EventStruct *event)
{
  ETH.end();
  stats_and_cache.processEvents();

  return true;
}

NWPluginData_static_runtime * NW004_data_struct_ETH_SPI::getNWPluginData_static_runtime() { return &stats_and_cache; }

void                          NW004_data_struct_ETH_SPI::onEvent(arduino_event_id_t   event,
                                                                 arduino_event_info_t info)
{
  switch (event)
  {
    case ARDUINO_EVENT_ETH_START:
      stats_and_cache.mark_start();
      break;
    case ARDUINO_EVENT_ETH_STOP:
      stats_and_cache.mark_stop();
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      stats_and_cache.mark_connected();
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      stats_and_cache.mark_disconnected();
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      stats_and_cache.mark_got_IP();
      break;
# if FEATURE_USE_IPV6
    case ARDUINO_EVENT_ETH_GOT_IP6:
      stats_and_cache.mark_got_IPv6(&info.got_ip6);
      break;
# endif // if FEATURE_USE_IPV6
    case ARDUINO_EVENT_ETH_LOST_IP:
      stats_and_cache.mark_lost_IP();
      break;

    default: break;
  }
}

} // namespace eth
} // namespace net
} // namespace ESPEasy

#endif // ifdef USES_NW004
