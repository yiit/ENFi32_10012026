#include "../NWPluginStructs/NW003_data_struct_ETH_RMII.h"

#ifdef USES_NW003

# include "../../../src/Globals/Settings.h"

# include "../../../src/Helpers/ESPEasy_time_calc.h"
# include "../../../src/Helpers/LongTermOnOffTimer.h"
# include "../../../src/Helpers/StringConverter.h"

# include "../../../src/WebServer/Markup.h"
# include "../../../src/WebServer/Markup_Forms.h"
# include "../../../src/WebServer/ESPEasy_key_value_store_webform.h"

# include "../Globals/NetworkState.h"

# include "../eth/ESPEasyEth.h"

# define NW_PLUGIN_ID  3
# define NW_PLUGIN_INTERFACE   ETH

namespace ESPEasy {
namespace net {
namespace eth {

static NWPluginData_static_runtime stats_and_cache(&NW_PLUGIN_INTERFACE);


// Keys as used in the Key-value-store
# define NW003_KEY_ETH_INDEX                         1
# define NW003_KEY_ETH_PHY_TYPE                      2
# define NW003_KEY_ETH_PHY_ADDR                      3
# define NW003_KEY_ETH_PIN_MDC                       4
# define NW003_KEY_ETH_PIN_MDIO                      5
# define NW003_KEY_ETH_PIN_POWER                     6
# define NW003_KEY_CLOCK_MODE                        7
# define NW003_KEY_IP                                8
# define NW003_KEY_GW                                9
# define NW003_KEY_SN                                10
# define NW003_KEY_DNS                               11

# define NW003_MAX_KEY                               12

const __FlashStringHelper * NW003_data_struct_ETH_RMII::getLabelString(uint32_t key, bool displayString, KVS_StorageType::Enum& storageType)
{
  storageType = KVS_StorageType::Enum::int8_type;

  switch (key)
  {
    case NW003_KEY_ETH_INDEX:  return F("Index");
    case NW003_KEY_ETH_PHY_TYPE: return displayString ? F("Ethernet PHY type") : F("phytype");
    case NW003_KEY_ETH_PHY_ADDR: return displayString ? F("Ethernet PHY Address") : F("phyaddr");
    case NW003_KEY_ETH_PIN_MDC: return displayString ? F("Ethernet MDC pin") : F("MDC");
    case NW003_KEY_ETH_PIN_MDIO: return displayString ? F("Ethernet MDIO pin") : F("MDIO");
    case NW003_KEY_ETH_PIN_POWER: return displayString ? F("Ethernet Power pin") : F("pwr");
    case NW003_KEY_CLOCK_MODE: return displayString ? F("Ethernet Clock") : F("clock");
    case NW003_KEY_IP:
      storageType = KVS_StorageType::Enum::string_type;
      return F("IP");
    case NW003_KEY_GW:
      storageType = KVS_StorageType::Enum::string_type;
      return displayString ? F("Gateway") : F("gw");
    case NW003_KEY_SN:
      storageType = KVS_StorageType::Enum::string_type;
      return displayString ? F("Subnetmask") : F("sn");
    case NW003_KEY_DNS:
      storageType = KVS_StorageType::Enum::string_type;
      return F("DNS");
  }
  return F("");
}

int32_t NW003_data_struct_ETH_RMII::getNextKey(int32_t key)
{
  if (key == -1) { return NW003_KEY_ETH_INDEX; }

  if (key >= NW003_MAX_KEY) { return -2; }
  return key + 1;
}

String NW003_data_struct_ETH_RMII::formatGpioLabel(uint32_t          key,
                                                   PinSelectPurpose& purpose,
                                                   bool              shortNotation)
{
  String label;

  if ((key >= NW003_KEY_ETH_PIN_MDC) && (key <= NW003_KEY_ETH_PIN_POWER))
  {
    KVS_StorageType::Enum storageType;
    purpose = PinSelectPurpose::Generic;
    label   = getLabelString(key, true, storageType);

    const bool optional = !shortNotation;

    switch (key)
    {
      case NW003_KEY_ETH_PIN_MDIO:
        purpose = PinSelectPurpose::Generic_input;
        label   = formatGpioName(
          label, gpio_direction::gpio_input, optional);
        break;
      case NW003_KEY_ETH_PIN_MDC:
      case NW003_KEY_ETH_PIN_POWER:
        purpose = PinSelectPurpose::Generic_output;
        label   = formatGpioName(
          label, gpio_direction::gpio_output, optional);
        break;
    }
  }
  return label;
}

WebFormItemParams NW003_makeWebFormItemParams(uint32_t key) {
  KVS_StorageType::Enum storageType;
  const __FlashStringHelper*label = NW003_data_struct_ETH_RMII::getLabelString(key, true, storageType);
  const __FlashStringHelper*id    = NW003_data_struct_ETH_RMII::getLabelString(key, false, storageType);

  return WebFormItemParams(label, id, storageType, key);
}

NW003_data_struct_ETH_RMII::NW003_data_struct_ETH_RMII(networkIndex_t networkIndex)
  : NWPluginData_base(nwpluginID_t(NW_PLUGIN_ID), networkIndex, &NW_PLUGIN_INTERFACE)
{
  stats_and_cache.clear(networkIndex);

  nw_event_id = Network.onEvent(NW003_data_struct_ETH_RMII::onEvent);
}

NW003_data_struct_ETH_RMII::~NW003_data_struct_ETH_RMII()
{
  if (nw_event_id != 0) {
    Network.removeEvent(nw_event_id);
  }
  nw_event_id = 0;
  stats_and_cache.processEvent_and_clear();
}

void NW003_data_struct_ETH_RMII::loadDefaults(ESPEasy_key_value_store     *kvs,
                                              ESPEasy::net::networkIndex_t networkIndex,
                                              ESPEasy::net::nwpluginID_t   nwPluginID)
{
  // Only load the defaults when the settings are empty
  if (kvs && kvs->isEmpty()) {
    if (isValid(Settings.ETH_Phy_Type) &&
        !ESPEasy::net::isSPI_EthernetType(Settings.ETH_Phy_Type))
    {
      kvs->setValue(NW003_KEY_ETH_INDEX,     static_cast<int8_t>(-1));
      kvs->setValue(NW003_KEY_ETH_PHY_TYPE,  static_cast<int8_t>(Settings.ETH_Phy_Type));
      kvs->setValue(NW003_KEY_ETH_PHY_ADDR,  static_cast<int8_t>(Settings.ETH_Phy_Addr));
      kvs->setValue(NW003_KEY_ETH_PIN_MDC,   static_cast<int8_t>(Settings.ETH_Pin_mdc_cs));
      kvs->setValue(NW003_KEY_ETH_PIN_MDIO,  static_cast<int8_t>(Settings.ETH_Pin_mdio_irq));
      kvs->setValue(NW003_KEY_ETH_PIN_POWER, static_cast<int8_t>(Settings.ETH_Pin_power_rst));
      kvs->setValue(NW003_KEY_CLOCK_MODE,    static_cast<int8_t>(Settings.ETH_Clock_Mode));

      // TODO TD-er: Copy IP info

      store_nwpluginTaskData_KVS(kvs, networkIndex, nwPluginID);
    }
    # if defined(DEFAULT_ETH_PHY_TYPE) && defined(DEFAULT_ETH_CLOCK_MODE) && defined(DEFAULT_ETH_PHY_ADDR) && \
    defined(DEFAULT_ETH_PIN_MDC) && defined(DEFAULT_ETH_PIN_MDIO)
    else if (isValid(DEFAULT_ETH_PHY_TYPE) &&
             !ESPEasy::net::isSPI_EthernetType(DEFAULT_ETH_PHY_TYPE))
    {
      kvs->setValue(NW003_KEY_ETH_INDEX,     static_cast<int8_t>(-1));
      kvs->setValue(NW003_KEY_ETH_PHY_TYPE,  static_cast<int8_t>(DEFAULT_ETH_PHY_TYPE));
      kvs->setValue(NW003_KEY_ETH_PHY_ADDR,  static_cast<int8_t>(DEFAULT_ETH_PHY_ADDR));
      kvs->setValue(NW003_KEY_ETH_PIN_MDC,   static_cast<int8_t>(DEFAULT_ETH_PIN_MDC));
      kvs->setValue(NW003_KEY_ETH_PIN_MDIO,  static_cast<int8_t>(DEFAULT_ETH_PIN_MDIO));
      kvs->setValue(NW003_KEY_ETH_PIN_POWER, static_cast<int8_t>(DEFAULT_ETH_PIN_POWER));
      kvs->setValue(NW003_KEY_CLOCK_MODE,    static_cast<int8_t>(DEFAULT_ETH_CLOCK_MODE));

      // TODO TD-er: Copy IP info

      store_nwpluginTaskData_KVS(kvs, networkIndex, nwPluginID);
    }
    # endif // if defined(DEFAULT_ETH_PHY_TYPE) && defined(DEFAULT_ETH_CLOCK_MODE) && defined(DEFAULT_ETH_PHY_ADDR) &&
    // defined(DEFAULT_ETH_PIN_MDC) && defined(DEFAULT_ETH_PIN_MDIO)
  }
}

void NW003_data_struct_ETH_RMII::webform_load(EventStruct *event)
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
    const int ids[] = {
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
    FormSelectorOptions selector(NR_ELEMENTS(ids), options, ids);
    auto params = NW003_makeWebFormItemParams(NW003_KEY_ETH_PHY_TYPE);
    params._defaultIntValue = static_cast<int>(EthPhyType_t::notSet);
    showFormSelector(*_kvs, selector, params);
  }
  {
    auto params = NW003_makeWebFormItemParams(NW003_KEY_ETH_PHY_ADDR);
    params._min             = -1;
    params._max             = 127;
    params._defaultIntValue = 1;
    showWebformItem(*_kvs, params);
    addFormNote(F("I&sup2;C-address of Ethernet PHY"
                  " (0 or 1 for LAN8720, 31 for TLK110, -1 autodetect)"
                  ));

  }
  {
    const int gpio_keys[] = {
      NW003_KEY_ETH_PIN_MDC,
      NW003_KEY_ETH_PIN_MDIO,
      NW003_KEY_ETH_PIN_POWER
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
  }
}

void NW003_data_struct_ETH_RMII::webform_save(EventStruct *event)
{
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

bool NW003_data_struct_ETH_RMII::webform_getPort(KeyValueWriter *writer) { return true; }

bool NW003_data_struct_ETH_RMII::init(EventStruct *event)
{
  auto data = getNWPluginData_static_runtime();

  if (data) {
    ESPEasy::net::eth::ETHConnectRelaxed(*data);
  }

  return true;
}

bool NW003_data_struct_ETH_RMII::exit(EventStruct *event) {
  ETH.end();
  stats_and_cache.processEvents();
  return true;
}

NWPluginData_static_runtime * NW003_data_struct_ETH_RMII::getNWPluginData_static_runtime() { return &stats_and_cache; }

void                          NW003_data_struct_ETH_RMII::onEvent(
  arduino_event_id_t   event,
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

#endif // ifdef USES_NW003
