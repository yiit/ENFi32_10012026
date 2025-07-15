#include "../NWPluginStructs/NW005_data_struct_PPP_modem.h"

#ifdef USES_NW005

# include "../Helpers/StringConverter.h"

# include "../WebServer/Markup_Forms.h"


# include <ESPEasySerialPort.h>
# include "../Helpers/_Plugin_Helper_serial.h"

// Keys as used in the Key-value-store
# define NW005_KEY_SERIAL_PORT 1
# define NW005_KEY_PIN_RX   2
# define NW005_KEY_PIN_TX   3
# define NW005_KEY_PIN_RTS   4
# define NW005_KEY_PIN_CTS   5
# define NW005_KEY_PIN_RESET  6
# define NW005_KEY_FLOWCTRL   10

const __FlashStringHelper* NW005_getLabelString(uint32_t key, bool displayString, ESPEasy_key_value_store::StorageType& storageType)
{
  storageType = ESPEasy_key_value_store::StorageType::int8_type;

  switch (key)
  {
    case NW005_KEY_SERIAL_PORT: return displayString ? F("Serial Port") : F("serPort");
    case NW005_KEY_PIN_RX: return F("RX");
    case NW005_KEY_PIN_TX: return F("TX");
    case NW005_KEY_PIN_RTS: return F("RTS");
    case NW005_KEY_PIN_CTS: return F("CTS");
    case NW005_KEY_PIN_RESET: return displayString ? F("Reset") : F("rst");
    case NW005_KEY_FLOWCTRL: return displayString ? F("Flow Control") : F("flowctrl");
  }
  return F("");
}

NW005_data_struct_PPP_modem::NW005_data_struct_PPP_modem(networkIndex_t networkIndex)
  : NWPluginData_base(nwpluginID_t(5), networkIndex)
{}

NW005_data_struct_PPP_modem::~NW005_data_struct_PPP_modem() {}

void NW005_data_struct_PPP_modem::webform_load(struct EventStruct *event)
{
  _load();
  {
    // TODO TD-er: We cannot use ESPEasySerialPort here as PPPClass needs to handle the pins using periman
    const int ids[] = {
      static_cast<int>(ESPEasySerialPort::serial0)
# if USABLE_SOC_UART_NUM > 1
      , static_cast<int>(ESPEasySerialPort::serial1)
# endif
# if USABLE_SOC_UART_NUM > 2
      , static_cast<int>(ESPEasySerialPort::serial2)
# endif
# if USABLE_SOC_UART_NUM > 3
      , static_cast<int>(ESPEasySerialPort::serial3)
# endif
# if USABLE_SOC_UART_NUM > 4
      , static_cast<int>(ESPEasySerialPort::serial4)
# endif
# if USABLE_SOC_UART_NUM > 5
      , static_cast<int>(ESPEasySerialPort::serial5)
# endif
    };

    constexpr int NR_ESPEASY_SERIAL_TYPES = NR_ELEMENTS(ids);
    const __FlashStringHelper*options[]   = {
      serialHelper_getSerialTypeLabel(ESPEasySerialPort::serial0)
# if USABLE_SOC_UART_NUM > 1
      , serialHelper_getSerialTypeLabel(ESPEasySerialPort::serial1)
# endif
# if USABLE_SOC_UART_NUM > 2
      , serialHelper_getSerialTypeLabel(ESPEasySerialPort::serial2)
# endif
# if USABLE_SOC_UART_NUM > 3
      , serialHelper_getSerialTypeLabel(ESPEasySerialPort::serial3)
# endif
# if USABLE_SOC_UART_NUM > 4
      , serialHelper_getSerialTypeLabel(ESPEasySerialPort::serial4)
# endif
# if USABLE_SOC_UART_NUM > 5
      , serialHelper_getSerialTypeLabel(ESPEasySerialPort::serial5)
# endif
    };

    int8_t port{};
    _kvs->getValue(NW005_KEY_SERIAL_PORT, port);
    FormSelectorOptions selector(NR_ESPEASY_SERIAL_TYPES, options, ids);
    selector.addFormSelector(F("Serial Port"), F("serPort"), port);

  }

  for (int i = NW005_KEY_PIN_RX; i <= NW005_KEY_PIN_RESET; ++i)
  {
    ESPEasy_key_value_store::StorageType storageType;
    const __FlashStringHelper *label = NW005_getLabelString(i, true, storageType);
    const __FlashStringHelper *id    = NW005_getLabelString(i, false, storageType);
    int8_t pin                       = -1;
    _kvs->getValue(i, pin);
    addFormPinSelect(PinSelectPurpose::Generic, label, id, pin);
  }
  uint8_t flow_ctrl{};

  _kvs->getValue(NW005_KEY_FLOWCTRL, flow_ctrl);

}

void NW005_data_struct_PPP_modem::webform_save(struct EventStruct *event)
{
  const uint32_t keys[] {
    NW005_KEY_SERIAL_PORT,
    NW005_KEY_PIN_RX,
    NW005_KEY_PIN_TX,
    NW005_KEY_PIN_RTS,
    NW005_KEY_PIN_CTS,
    NW005_KEY_PIN_RESET,
    NW005_KEY_FLOWCTRL
  };


  for (int i = 0; i < NR_ELEMENTS(keys); ++i)
  {
    ESPEasy_key_value_store::StorageType storageType;
    const __FlashStringHelper *id = NW005_getLabelString(keys[i], false, storageType);
    _kvs->setValue(storageType, keys[i], webArg(id));
  }
  _store();
}

struct testStruct {
  String   foo1 = F("test123");
  int64_t  foo2 = -123;
  uint32_t foo3 = 123;


};

void NW005_data_struct_PPP_modem::testWrite()
{
  if (!_KVS_initialized()) { return; }

  for (int i = 0; i < 30; ++i) {
    testStruct _test{ .foo1 = concat(F("str_"), (3 * (i) + 1)), .foo2 = -1l * (3 * (i) + 2), .foo3 = 3 * (i) + 3 };
    _kvs->setValue(3 * (i) + 1, _test.foo1);
    _kvs->setValue(3 * (i) + 2, _test.foo2);
    _kvs->setValue(3 * (i) + 3, _test.foo3);
  }

  _kvs->dump();


  //  _kvs->clear();
  _store();

  _load();

  _kvs->dump();

}

void NW005_data_struct_PPP_modem::testRead()
{
  if (!_KVS_initialized()) { return; }
  _load();

  for (uint32_t i = 1; i <= 30; ++i) {
    String val;
    _kvs->getValue(i, val);
    addLog(LOG_LEVEL_INFO, strformat(F("KVS, foo%d: %s"), i, val.c_str()));
  }

}

#endif // ifdef USES_NW005
