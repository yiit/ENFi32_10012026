#include "../NWPluginStructs/NW005_data_struct_PPP_modem.h"

#ifdef USES_NW005

# include "../../../src/Globals/Settings.h"

# include "../../../src/Helpers/ESPEasy_time_calc.h"
# include "../../../src/Helpers/LongTermOnOffTimer.h"
# include "../../../src/Helpers/StringConverter.h"
# include "../../../src/Helpers/_Plugin_Helper_serial.h"

# include "../../../src/WebServer/HTML_wrappers.h"
# include "../../../src/WebServer/Markup.h"
# include "../../../src/WebServer/Markup_Forms.h"
# include "../../../src/WebServer/ESPEasy_key_value_store_webform.h"

# include <ESPEasySerialPort.h>
# include <PPP.h>

// # include <esp_modem_api.h>
// #include <esp_modem_c_api_types.h>

# define NW_PLUGIN_ID  5

namespace ESPEasy {
namespace net {
namespace ppp {


static LongTermOnOffTimer _startStopStats;
static LongTermOnOffTimer _connectedStats;
static LongTermOnOffTimer _gotIPStats;
# if FEATURE_USE_IPV6
static LongTermOnOffTimer _gotIP6Stats;
# endif
static IPAddress _dns_cache[2]{};


// Keys as used in the Key-value-store
# define NW005_KEY_SERIAL_PORT          1
# define NW005_KEY_PIN_RX               2
# define NW005_KEY_PIN_TX               3
# define NW005_KEY_PIN_RTS              4
# define NW005_KEY_PIN_CTS              5
# define NW005_KEY_PIN_RESET            6
# define NW005_KEY_PIN_RESET_ACTIVE_LOW 7
# define NW005_KEY_PIN_RESET_DELAY      8
# define NW005_KEY_BAUDRATE             9
# define NW005_KEY_FLOWCTRL             10
# define NW005_KEY_MODEM_MODEL          11
# define NW005_KEY_APN                  12
# define NW005_KEY_SIM_PIN              13
# define NW005_KEY_SET_DEFAULT_ROUTE    14

const __FlashStringHelper* NW005_getLabelString(uint32_t key, bool displayString, ESPEasy_key_value_store::StorageType& storageType)
{
  storageType = ESPEasy_key_value_store::StorageType::int8_type;

  switch (key)
  {
    case NW005_KEY_SERIAL_PORT:
      return displayString ? F("Serial Port") : F("serPort");
    case NW005_KEY_PIN_RX: return F("RX");
    case NW005_KEY_PIN_TX: return F("TX");
    case NW005_KEY_PIN_RTS: return F("RTS");
    case NW005_KEY_PIN_CTS: return F("CTS");
    case NW005_KEY_PIN_RESET: return displayString ? F("Reset") : F("rst");
    case NW005_KEY_PIN_RESET_ACTIVE_LOW:
      storageType = ESPEasy_key_value_store::StorageType::bool_type;
      return displayString ? F("Reset Active Low") : F("rst_act_low");
    case NW005_KEY_PIN_RESET_DELAY:
      storageType = ESPEasy_key_value_store::StorageType::uint16_type;
      return displayString ? F("Reset Delay") : F("rst_delay");
    case NW005_KEY_BAUDRATE:
      storageType = ESPEasy_key_value_store::StorageType::uint32_type;
      return displayString ? F("Baud rate") : F("baudrate");
    case NW005_KEY_FLOWCTRL:
      return displayString ? F("Flow Control") : F("flowctrl");
    case NW005_KEY_MODEM_MODEL: return displayString ? F("Modem Model") : F("mmodel");
    case NW005_KEY_APN:
      storageType = ESPEasy_key_value_store::StorageType::string_type;
      return displayString ? F("Access Point Name (APN)") : F("apn");
    case NW005_KEY_SIM_PIN:
      storageType = ESPEasy_key_value_store::StorageType::string_type;
      return displayString ? F("SIM Card Pin") : F("pin");
    case NW005_KEY_SET_DEFAULT_ROUTE:
      storageType = ESPEasy_key_value_store::StorageType::bool_type;
      return displayString ? F("Set as Default Route") : F("default_route");


  }
  return F("");
}

NW005_data_struct_PPP_modem::NW005_data_struct_PPP_modem(networkIndex_t networkIndex)
  : NWPluginData_base(nwpluginID_t(NW_PLUGIN_ID), networkIndex)
{
  _connectedStats.clear();
  _gotIPStats.clear();
# if FEATURE_USE_IPV6
  _gotIP6Stats.clear();
# endif
  nw_event_id = Network.onEvent(NW005_data_struct_PPP_modem::onEvent);
}

WebFormItemParams NW005_makeWebFormItemParams(uint32_t key) {
  ESPEasy_key_value_store::StorageType storageType;
  const __FlashStringHelper*label = NW005_getLabelString(key, true, storageType);
  const __FlashStringHelper*id    = NW005_getLabelString(key, false, storageType);

  return WebFormItemParams(label, id, storageType, key);
}

NW005_data_struct_PPP_modem::~NW005_data_struct_PPP_modem() {
  if (_modem_task_data.modem_taskHandle) {
    vTaskDelete(_modem_task_data.modem_taskHandle);
    _modem_task_data.modem_taskHandle = NULL;
  }
  PPP.mode(ESP_MODEM_MODE_COMMAND);
  PPP.end();
  _modem_task_data.modem_initialized = false;

  if (nw_event_id != 0) {
    Network.removeEvent(nw_event_id);
  }
  nw_event_id = 0;
}

enum class NW005_modem_model {
  generic = 1,
  SIM7600 = 2,
  SIM7070 = 3,
  SIM7000 = 4,
  BG96    = 5,
  SIM800  = 6

};

const __FlashStringHelper* toString(NW005_modem_model NW005_modemmodel)
{
  switch (NW005_modemmodel)
  {
    case NW005_modem_model::generic: return F("Generic");
    case NW005_modem_model::SIM7600: return F("SIM7600");
    case NW005_modem_model::SIM7070: return F("SIM7070");
    case NW005_modem_model::SIM7000: return F("SIM7000");
    case NW005_modem_model::BG96:    return F("BG96");
    case NW005_modem_model::SIM800:  return F("SIM800");
  }
  return F("unknown");
}

ppp_modem_model_t to_ppp_modem_model_t(NW005_modem_model NW005_modemmodel)
{
  switch (NW005_modemmodel)
  {
    case NW005_modem_model::generic: return PPP_MODEM_GENERIC;
    case NW005_modem_model::SIM7600: return PPP_MODEM_SIM7600;
    case NW005_modem_model::SIM7070: return PPP_MODEM_SIM7070;
    case NW005_modem_model::SIM7000: return PPP_MODEM_SIM7000;
    case NW005_modem_model::BG96:    return PPP_MODEM_BG96;
    case NW005_modem_model::SIM800:  return PPP_MODEM_SIM800;
  }
  return PPP_MODEM_GENERIC;
}

String NW005_data_struct_PPP_modem::getRSSI() const
{
  if (!_modem_task_data.modem_initialized) { return F("-"); }
  const int rssi_raw = PPP.RSSI();

  if (rssi_raw == 99) { return F("-"); } // Not known or not detectable

  if (rssi_raw == 0) { return F("<= -113"); }

  if (rssi_raw >= 31) { return F(">= -51"); }
  return String(map(rssi_raw, 2, 30, -109, -53));
}

String NW005_data_struct_PPP_modem::getBER() const
{
  if (_modem_task_data.modem_initialized) {
    switch (PPP.BER())
    {
      case 0: return F("<0.01 %");
      case 1: return F("0.01 % ... 0.1 %");
      case 2: return F("0.1 % ... 0.5 %");
      case 3: return F("0.5 % ... 1 %");
      case 4: return F("1 % ... 2 %");
      case 5: return F("2 % ... 4 %");
      case 6: return F("4 % ... 8 %");
      case 7: return F(">= 8 %");
      case 99: break; // Not known or not detectable
    }
  }
  return F("-");
}

bool NW005_data_struct_PPP_modem::attached() const
{
  if (!_modem_task_data.modem_initialized) { return false; }
  return PPP.attached();
}

String NW005_data_struct_PPP_modem::IMEI() const
{
  if (!_modem_task_data.modem_initialized) { return EMPTY_STRING; }
  return PPP.IMEI();
}

String NW005_data_struct_PPP_modem::operatorName() const
{
  if (!_modem_task_data.modem_initialized) { return EMPTY_STRING; }
  return PPP.operatorName();
}

const __FlashStringHelper* NW005_decode_label(int sysmode_index, uint8_t i, String& value_str)
{
  const __FlashStringHelper*res = F("");

  switch (sysmode_index)
  {
    case 0:
    {
      // No service system mode
      const __FlashStringHelper*labels[]
      {
        F("System Mode"), F("Operation Mode")
      };

      if (i < NR_ELEMENTS(labels)) { res = labels[i]; }
      break;
    }
    case 1:
    {
      // GSM system mode
      const __FlashStringHelper*labels[]
      {
        F("System Mode"), F("Operation Mode"), F("MCC-MNC"), F("LAC"), F("Cell ID"), F("Absolute RF Ch Num"), F("RxLev"),
        F("Track LO Adjust"), F("C1-C2")
      };

      if (i < NR_ELEMENTS(labels)) { res = labels[i]; }
      break;
    }
    case 2:
    {
      // WCDMA system mode
      const __FlashStringHelper*labels[]
      {
        F("System Mode"), F("Operation Mode"), F("MCC-MNC"), F("LAC"), F("Cell ID"), F("Frequency Band"), F("PSC"), F("Freq"), F("SSC"),
        F("EC/IO"), F("RSCP"), F("Qual"), F("RxLev"), F("TXPWR")
      };

      if (i < NR_ELEMENTS(labels)) { res = labels[i]; }
      break;
    }
    case 3:
    {
      // LTE system mode
      const __FlashStringHelper*labels[]
      {
        F("System Mode"), F("Operation Mode"), F("MCC-MNC"), F("TAC"), F("SCellID"), F("PCellID"), F("Frequency Band"), F("earfcn"),
        F("dlbw"), F("ulbw"), F("RSRQ"), F("RSRP"), F("RSSI"), F("RSSNR")
      };

      if (i < NR_ELEMENTS(labels)) {
        res = labels[i];

        switch (i)
        {
          case 10:
            // RSRQ
          {
            float val = value_str.toInt();
            val      -= 40;
            val      /= 2.0f;
            value_str = String(val, 1) + F(" [dBm]");
            break;
          }
          case 11:
            // RSRP
            value_str = strformat(F("%d [dBm]"), value_str.toInt() - 140);
            break;
          case 12:
            // RSSI
            value_str = strformat(F("%d [dBm]"), value_str.toInt() - 110);
            break;
        }

      }
      break;
    }
  }
  return res;
}

void NW005_data_struct_PPP_modem::webform_load_UE_system_information()
{
  if (!PPP.attached()) {
    // clear cached string
    _modem_task_data.AT_CPSI.clear();
    return;
  }

  String res = write_AT_cmd(F("AT+CPSI?"), 1000);

  if (res.isEmpty()) { res = _modem_task_data.AT_CPSI; }

  if (!res.isEmpty() /* && res.startsWith(F("+CPSI"))*/) {
    int start_index         = 0;
    int end_index           = res.indexOf(',');
    const String systemMode = res.substring(start_index, end_index);
    addLog(LOG_LEVEL_INFO, concat(F("PPP: UE sysinfo: "), systemMode));

    const __FlashStringHelper*sysmode_str[] = {
      F("NO SERVICE"), F("GSM"), F("WCDMA"), F("LTE")
    };
    int sysmode_index = -1;

    for (int i = 0; i < NR_ELEMENTS(sysmode_str) && sysmode_index == -1; ++i) {
      if (systemMode.endsWith(sysmode_str[i])) { sysmode_index = i; }

    }

    if (sysmode_index == -1) { return; }

    // Update cached string
    _modem_task_data.AT_CPSI = res;

    addFormSubHeader(F("UE System Information"));

    res += ','; // Add trailing comma so we're not missing the last element

    for (int i = 0; start_index < res.length() && end_index != -1 && i < 15; ++i)
    {
      String value_str   = res.substring(start_index, end_index);
      const String label = NW005_decode_label(sysmode_index, i, value_str);

      if (!label.isEmpty()) {
        addRowLabel(label);

        if (i == 0) {
          // We have some leading characters left here, so use the 'clean' strings
          addHtml(sysmode_str[sysmode_index]);
        } else {
          addHtml_pre(value_str);
        }
      }
      start_index = end_index + 1;
      end_index   = res.indexOf(',', start_index);
    }

  }

}

void NW005_data_struct_PPP_modem::webform_load(EventStruct *event)
{
  _load();

  showWebformItem(
    *_kvs,
    NW005_makeWebFormItemParams(NW005_KEY_SET_DEFAULT_ROUTE));


  addFormSubHeader(F("Device Settings"));
  {
    const int ids[] = {
      static_cast<int>(NW005_modem_model::generic),
      static_cast<int>(NW005_modem_model::SIM7600),
      static_cast<int>(NW005_modem_model::SIM7070),
      static_cast<int>(NW005_modem_model::SIM7000),
      static_cast<int>(NW005_modem_model::BG96),
      static_cast<int>(NW005_modem_model::SIM800)
    };

    const __FlashStringHelper*options[] = {
      toString(NW005_modem_model::generic),
      toString(NW005_modem_model::SIM7600),
      toString(NW005_modem_model::SIM7070),
      toString(NW005_modem_model::SIM7000),
      toString(NW005_modem_model::BG96),
      toString(NW005_modem_model::SIM800)
    };

    FormSelectorOptions selector(NR_ELEMENTS(ids), options, ids);
    showFormSelector(*_kvs, selector, NW005_makeWebFormItemParams(NW005_KEY_MODEM_MODEL));
  }

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

    FormSelectorOptions selector(NR_ELEMENTS(ids), options, ids);
    showFormSelector(*_kvs, selector, NW005_makeWebFormItemParams(NW005_KEY_SERIAL_PORT));
  }

  for (int i = NW005_KEY_PIN_RX; i <= NW005_KEY_PIN_RESET; ++i)
  {
    ESPEasy_key_value_store::StorageType storageType;
    const __FlashStringHelper *id = NW005_getLabelString(i, false, storageType);
    PinSelectPurpose purpose      = PinSelectPurpose::Generic;
    String label                  = NW005_formatGpioLabel(i, purpose);

    int8_t pin = -1;
    _kvs->getValue(i, pin);

    addFormPinSelect(purpose, label, id, pin);
  }
  showWebformItem(
    *_kvs,
    NW005_makeWebFormItemParams(NW005_KEY_PIN_RESET_ACTIVE_LOW));
  {
    auto params = NW005_makeWebFormItemParams(NW005_KEY_PIN_RESET_DELAY);
    params._max             = 2000;
    params._defaultIntValue = 200;
    showWebformItem(*_kvs, params);
    addUnit(F("ms"));

  }
  {
    const int ids[] = {
      ESP_MODEM_FLOW_CONTROL_NONE,
      ESP_MODEM_FLOW_CONTROL_SW,
      ESP_MODEM_FLOW_CONTROL_HW
    };
    const __FlashStringHelper*options[] = {
      F("None"),
      F("Software"),
      F("Hardware")
    };

    FormSelectorOptions selector(NR_ELEMENTS(ids), options, ids);
    showFormSelector(*_kvs, selector, NW005_makeWebFormItemParams(NW005_KEY_FLOWCTRL));
    addFormNote(F("Only set flow control if RTS and CTS are used"));
  }
  {
    auto params = NW005_makeWebFormItemParams(NW005_KEY_BAUDRATE);
    params._max             = 10000000;
    params._defaultIntValue = 115200;
    showWebformItem(*_kvs, params);
  }

  addFormSubHeader(F("Connection"));
  {
    auto params = NW005_makeWebFormItemParams(NW005_KEY_APN);
    params._maxLength = 64;
    showWebformItem(*_kvs, params);
  }
  {
    auto params = NW005_makeWebFormItemParams(NW005_KEY_SIM_PIN);
    params._maxLength = 4;
    showWebformItem(*_kvs, params);
    addFormNote(F("Only numerical digits"));
  }

  if (!Settings.getNetworkEnabled(event->NetworkIndex)) { return; }

  addFormSubHeader(F("Modem State"));
  addRowLabel(F("Modem Model"));

  if (!_modem_task_data.modem_initialized) {
    if (_modem_task_data.initializing) {
      addHtml(F("Initializing ..."));
    } else {
      addHtml(F("Not found"));
    }
    return;
  }

  addHtml_pre(PPP.moduleName());
  addRowLabel(F("IMEI"));
  addHtml_pre(PPP.IMEI());

  addRowLabel(F("IMSI"));
  addHtml_pre(PPP.IMSI());

  if (PPP.attached()) {
    const String operatorName = PPP.operatorName();
    addRowLabel(F("Mobile Country Code (MCC)"));
    addHtml_pre(operatorName.substring(0, 3));
    addRowLabel(F("Mobile Network Code (MNC)"));
    addHtml_pre(operatorName.substring(3));
    addFormNote(F("See <a href=\"https://en.wikipedia.org/wiki/Mobile_country_code\">Wikipedia - Mobile Country Code</a>"));

    if (PPP.mode() != ESP_MODEM_MODE_CMUX) {
      PPP.mode(ESP_MODEM_MODE_CMUX);
    }

    /*
       {
       String res = PPP.cmd("AT+CPSI?", 1000);

       if (!res.isEmpty()) {
        addRowLabel(F("AT+CPSI?"));
        addHtml_pre(res);
       }
       }*/
    /*
       {
       String res = PPP.cmd("AT+CPSITD?", 1000);

       if (!res.isEmpty()) {
         addRowLabel(F("AT+CPSITD?"));
         addHtml_pre(res);
       }
       }
     */

  }

  addRowLabel(F("RSSI"));
  addHtml(getRSSI());
  addUnit(F("dBm"));

  addRowLabel(F("BER"));
  addHtml(getBER());

  addRowLabel(F("Radio State"));
  addHtml((PPP.radioState() == 0) ? F("Minimal") : F("Full"));

  int networkMode = PPP.networkMode();

  if (networkMode >= 0) {
    addRowLabel(F("Network Mode"));

    // Result from AT+CNSMOD Show network system mode
    switch (networkMode)
    {
      case 0: addHtml(F("no service"));
        break;
      case 1: addHtml(F("GSM"));
        break;
      case 2: addHtml(F("GPRS"));
        break;
      case 3: addHtml(F("EGPRS (EDGE)"));
        break;
      case 4: addHtml(F("WCDMA"));
        break;
      case 5: addHtml(F("HSDPA only(WCDMA)"));
        break;
      case 6: addHtml(F("HSUPA only(WCDMA)"));
        break;
      case 7: addHtml(F("HSPA (HSDPA and HSUPA, WCDMA)"));
        break;
      case 8: addHtml(F("LTE"));
        break;
      default: addHtmlInt(networkMode);
        break;
    }
  }

  {
    int batStatus = PPP.batteryStatus();
    addRowLabel(F("Battery Status"));
    const __FlashStringHelper*str = F("Not Available");

    switch (batStatus)
    {
      case 0: str = F("Not Charging");
        break;
      case 1: str = F("Charging");
        break;
      case 2: str = F("Charging Done");
        break;
    }
    addHtml(str);

    if (batStatus >= 0) {
      int batVolt = PPP.batteryVoltage();

      if (batVolt >= 0) {
        addRowLabel(F("Battery Voltage"));
        addHtmlInt(batVolt);
        addUnit(F("mV"));
      }

      int batLevel = PPP.batteryLevel();

      if (batLevel >= 0) {
        addRowLabel(F("Battery Level"));
        addHtmlInt(batLevel);
      }
    }
  }

  // TODO TD-er: Disabled for now, missing PdpContext

  /*
     esp_modem_dce_t *handle = PPP.handle();

     if (handle) {
     int state{};

     if (esp_modem_get_network_registration_state(handle, state) == ESP_OK) {
      addRowLabel(F("Network Registration State"));
      const __FlashStringHelper*str = F("Unknown");

      switch (state)
      {
        case 0: str = F("Not registered, MT is not currently searching an operator to register to");
          break;
        case 1: str = F("Registered, home network");
          break;
        case 2: str = F("Not registered, but MT is currently trying to attach or searching an operator to register to");
          break;
        case 3: str = F("Registration denied");
          break;
        case 4: str = F("Unknown");
          break;
        case 5: str = F("Registered, Roaming");
          break;
        case 6: str = F("Registered, for SMS only, home network (NB-IoT only)");
          break;
        case 7: str = F("Registered, for SMS only, roaming (NB-IoT only)");
          break;
        case 8: str = F("Attached for emergency bearer services only");
          break;
        case 9: str = F("Registered for CSFB not preferred, home network");
          break;
        case 10: str = F("Registered for CSFB not preferred, roaming");
          break;
      }
      addHtml(str);
     }
     }
   */

  webform_load_UE_system_information();

  if (PPP.attached()) {
    PPP.mode(ESP_MODEM_MODE_DATA);
  }
}

void NW005_data_struct_PPP_modem::webform_save(EventStruct *event)
{
  const uint32_t keys[] {
    NW005_KEY_SERIAL_PORT,
    NW005_KEY_PIN_RX,
    NW005_KEY_PIN_TX,
    NW005_KEY_PIN_RTS,
    NW005_KEY_PIN_CTS,
    NW005_KEY_PIN_RESET,
    NW005_KEY_PIN_RESET_ACTIVE_LOW,
    NW005_KEY_PIN_RESET_DELAY,
    NW005_KEY_BAUDRATE,
    NW005_KEY_FLOWCTRL,
    NW005_KEY_MODEM_MODEL,
    NW005_KEY_APN,
    NW005_KEY_SIM_PIN,
    NW005_KEY_SET_DEFAULT_ROUTE
  };


  for (int i = 0; i < NR_ELEMENTS(keys); ++i)
  {
    ESPEasy_key_value_store::StorageType storageType;
    const __FlashStringHelper *id = NW005_getLabelString(keys[i], false, storageType);
    storeWebformItem(*_kvs, keys[i], storageType, id);
  }
  _store();
}

bool NW005_data_struct_PPP_modem::webform_getPort(String& str)
{
  str.clear();

  if (_KVS_initialized() && _load()) {
    int serialPort = _kvs->getValueAsInt_or_default(NW005_KEY_SERIAL_PORT, -1);
    str = serialHelper_getSerialTypeLabel(static_cast<ESPEasySerialPort>(serialPort));

    if (serialPort >= 0) {
      const uint32_t keys[] {
        NW005_KEY_PIN_RX,
        NW005_KEY_PIN_TX,
        NW005_KEY_PIN_RTS,
        NW005_KEY_PIN_CTS,
        NW005_KEY_PIN_RESET
      };

      for (int i = 0; i < NR_ELEMENTS(keys); ++i) {
        int pin = _kvs->getValueAsInt_or_default(keys[i], -1);

        if (pin >= 0) {
          PinSelectPurpose purpose = PinSelectPurpose::Generic;
          const bool shortNotation = true;
          String     label         = NW005_formatGpioLabel(keys[i], purpose, shortNotation);
          str += strformat(F("\n%s: %d"),
                           label.c_str(),
                           pin);
        }
      }
    }
  }
  return !str.isEmpty();
}

void NW005_begin_modem_task(void *parameter)
{
  NW005_modem_task_data*modem_task_data = static_cast<NW005_modem_task_data *>(parameter);

  if (!modem_task_data->modem_initialized) {
    modem_task_data->initializing = true;
    const bool res =
      PPP.begin(
        modem_task_data->model,
        modem_task_data->uart_num,
        modem_task_data->baud_rate);
    modem_task_data->initializing = false;
    modem_task_data->logString    =  strformat(
      F("PPP: Module Name: %s, IMEI: %s"),
      PPP.moduleName().c_str(),
      PPP.IMEI().c_str());

    if (res) {

      uint32_t start = millis();

      do
      {
        delay(100);
      }
      while (timePassedSince(start) < 5000 && !PPP.attached());

      PPP.mode(ESP_MODEM_MODE_CMUX);
      modem_task_data->AT_CPSI = PPP.cmd(F("AT+CPSI?"), 3000);
    }
    modem_task_data->modem_initialized = res;
  }
  modem_task_data->modem_taskHandle = NULL;
  vTaskDelete(modem_task_data->modem_taskHandle);
}

bool NW005_data_struct_PPP_modem::init(EventStruct *event)
{
  if (!_KVS_initialized()) {
    addLog(LOG_LEVEL_ERROR, F("PPP: Could not initialize storage"));
    return false;
  }

  if (!_load()) {
    addLog(LOG_LEVEL_ERROR, F("PPP: Could not load settings"));
    return false;
  }

  PPP.setResetPin(
    _kvs->getValueAsInt_or_default(NW005_KEY_PIN_RESET, -1),
    _kvs->getValueAsInt_or_default(NW005_KEY_PIN_RESET_ACTIVE_LOW, 0),
    _kvs->getValueAsInt_or_default(NW005_KEY_PIN_RESET_DELAY, 200));

  const int rtsPin               = _kvs->getValueAsInt_or_default(NW005_KEY_PIN_RTS, -1);
  const int ctsPin               = _kvs->getValueAsInt_or_default(NW005_KEY_PIN_CTS, -1);
  esp_modem_flow_ctrl_t flow_ctl = static_cast<esp_modem_flow_ctrl_t>(_kvs->getValueAsInt_or_default(NW005_KEY_FLOWCTRL,
                                                                                                     ESP_MODEM_FLOW_CONTROL_NONE));

  if ((rtsPin < 0) || (ctsPin < 0)) {
    if (flow_ctl != ESP_MODEM_FLOW_CONTROL_NONE) {
      addLog(LOG_LEVEL_INFO, F("PPP: Disable flow control as RTS/CTS are not set"));
      flow_ctl = ESP_MODEM_FLOW_CONTROL_NONE;
    }
  }

  if (!PPP.setPins(
        _kvs->getValueAsInt_or_default(NW005_KEY_PIN_TX, -1),
        _kvs->getValueAsInt_or_default(NW005_KEY_PIN_RX, -1),
        rtsPin,
        ctsPin,
        flow_ctl))
  {
    addLog(LOG_LEVEL_ERROR, F("PPP: Could not set pins"));
    return false;
  }
  {
    String apn;
    _kvs->getValue(NW005_KEY_APN, apn);
    PPP.setApn(apn.c_str());
  }
  {
    String pin;
    _kvs->getValue(NW005_KEY_SIM_PIN, pin);

    if (pin.length() >= 4) {
      PPP.setPin(pin.c_str());
    }
  }

  _modem_task_data.model = to_ppp_modem_model_t(static_cast<NW005_modem_model>(
                                                  _kvs->getValueAsInt_or_default(NW005_KEY_MODEM_MODEL, PPP_MODEM_GENERIC)));

  _modem_task_data.uart_num = 1;
  {
    int8_t serial = 1;

    if (_kvs->getValue(NW005_KEY_SERIAL_PORT, serial)) {
      switch (static_cast<ESPEasySerialPort>(serial))
      {
        case ESPEasySerialPort::serial0: _modem_task_data.uart_num = 0;
          break;
# if USABLE_SOC_UART_NUM > 1
        case ESPEasySerialPort::serial1: _modem_task_data.uart_num = 1;
          break;
# endif // if USABLE_SOC_UART_NUM > 1
# if USABLE_SOC_UART_NUM > 2
        case ESPEasySerialPort::serial2: _modem_task_data.uart_num = 2;
          break;
# endif // if USABLE_SOC_UART_NUM > 2
# if USABLE_SOC_UART_NUM > 3
        case ESPEasySerialPort::serial3: _modem_task_data.uart_num = 3;
          break;
# endif // if USABLE_SOC_UART_NUM > 3
# if USABLE_SOC_UART_NUM > 4
        case ESPEasySerialPort::serial4: _modem_task_data.uart_num = 4;
          break;
# endif // if USABLE_SOC_UART_NUM > 4
# if USABLE_SOC_UART_NUM > 5
        case ESPEasySerialPort::serial5: _modem_task_data.uart_num = 5;
          break;
# endif // if USABLE_SOC_UART_NUM > 5
        default: break;
      }
    }
  }
  _modem_task_data.baud_rate         = _kvs->getValueAsInt_or_default(NW005_KEY_BAUDRATE, 115200);
  _modem_task_data.modem_initialized = false;
  _modem_task_data.initializing      = false;

  xTaskCreatePinnedToCore(
    NW005_begin_modem_task,             // Function that should be called
    "PPP.begin()",                      // Name of the task (for debugging)
    4000,                               // Stack size (bytes)
    &_modem_task_data,                  // Parameter to pass
    1,                                  // Task priority
    &_modem_task_data.modem_taskHandle, // Task handle
    xPortGetCoreID()                    // Core you want to run the task on (0 or 1)
    );

  return true;
}

bool NW005_data_struct_PPP_modem::exit(EventStruct *event)
{
  if (_modem_task_data.modem_taskHandle) {
    vTaskDelete(_modem_task_data.modem_taskHandle);
    _modem_task_data.modem_taskHandle = NULL;
  }
  PPP.mode(ESP_MODEM_MODE_COMMAND);
  PPP.end();
  _modem_task_data.modem_initialized = false;
  return true;
}

bool NW005_data_struct_PPP_modem::handle_priority_route_changed()
{
  bool res{};

  if (PPP.isDefault()) {
    // Check to see if we may need to restore any cached DNS server
    for (size_t i = 0; i < NR_ELEMENTS(_dns_cache); ++i) {
      auto tmp = PPP.dnsIP(i);

      if ((_dns_cache[i] != INADDR_NONE) && (_dns_cache[i] != tmp)) {
        addLog(LOG_LEVEL_INFO, strformat(
                 F("NW005: Restore cached DNS server %d from %s to %s"),
                 i,
                 tmp.toString().c_str(),
                 _dns_cache[i].toString().c_str()
                 ));

        PPP.dnsIP(i, _dns_cache[i]);
        res = true;
      }
    }
  }
  return res;
}

String NW005_data_struct_PPP_modem::write_AT_cmd(const String& cmd, int timeout)
{
  String res;
  auto   cur_mode = PPP.mode();

  if (PPP.mode() != ESP_MODEM_MODE_CMUX) {
    PPP.mode(ESP_MODEM_MODE_CMUX);
    res = PPP.cmd(cmd.c_str(), timeout);
  }
  PPP.mode(cur_mode);
  return res;
}

LongTermTimer::Duration NW005_data_struct_PPP_modem::getConnectedDuration_ms() const
{
  return _connectedStats.getLastOnDuration_ms();
}

void NW005_data_struct_PPP_modem::onEvent(arduino_event_id_t event, arduino_event_info_t info) {
  // TODO TD-er: Must store flags from events in static (or global) object to act on it later.
  switch (event)
  {
    case ARDUINO_EVENT_PPP_START:     addLog(LOG_LEVEL_INFO, F("PPP Started"));
      _startStopStats.setOn();
# if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0)
      PPP.setRoutePrio(200);
# endif
      break;
    case ARDUINO_EVENT_PPP_STOP:      addLog(LOG_LEVEL_INFO, F("PPP Stopped"));
      _startStopStats.setOff();
      break;
    case ARDUINO_EVENT_PPP_CONNECTED:
      _connectedStats.setOn();
      addLog(LOG_LEVEL_INFO, F("PPP Connected"));

      break;
    case ARDUINO_EVENT_PPP_DISCONNECTED:
      _connectedStats.setOff();
      addLog(LOG_LEVEL_INFO, concat(
               F("PPP Disconnected. Connected for: "),
               format_msec_duration_HMS(_connectedStats.getLastOnDuration_ms())));
      WiFi.AP.enableNAPT(false);
      break;
    case ARDUINO_EVENT_PPP_GOT_IP:

      for (size_t i = 0; i < NR_ELEMENTS(_dns_cache); ++i) {
        auto tmp = PPP.dnsIP(i);

        if (tmp != INADDR_NONE) {
          _dns_cache[i] = tmp;
          addLog(LOG_LEVEL_INFO, strformat(F("DNS Cache %d set to %s"), i, tmp.toString(true).c_str()));
        }

      }

      _gotIPStats.setOn();
      addLog(LOG_LEVEL_INFO, F("PPP Got IP"));

      //      PPP.setDefault();

      if (WiFi.AP.enableNAPT(true)) {
        addLog(LOG_LEVEL_INFO, F("WiFi.AP.enableNAPT"));
      }
      break;
# if FEATURE_USE_IPV6
    case ARDUINO_EVENT_PPP_GOT_IP6:
      _gotIP6Stats.setOn();
      addLog(LOG_LEVEL_INFO, F("PPP Got IPv6"));
      break;
# endif // if FEATURE_USE_IPV6
    case ARDUINO_EVENT_PPP_LOST_IP:
      _gotIPStats.setOff();
# if FEATURE_USE_IPV6
      _gotIP6Stats.setOff();
# endif
      addLog(LOG_LEVEL_INFO, F("PPP Lost IP"));
      WiFi.AP.enableNAPT(false);
      break;

    default: break;
  }
}

String NW005_data_struct_PPP_modem::NW005_formatGpioLabel(uint32_t key, PinSelectPurpose& purpose, bool shortNotation) const
{
  String label;

  if ((key >= NW005_KEY_PIN_RX) && (key <= NW005_KEY_PIN_RESET))
  {
    ESPEasy_key_value_store::StorageType storageType;
    purpose = PinSelectPurpose::Generic;
    label   = NW005_getLabelString(key, true, storageType);

    const bool optional = !shortNotation;

    switch (key)
    {
      case NW005_KEY_PIN_RX:
        purpose = PinSelectPurpose::Serial_input;
        label   = shortNotation
          ? formatGpioName_RX(false)
          : formatGpioName_serialRX(false);
        break;
      case NW005_KEY_PIN_TX:
        purpose = PinSelectPurpose::Serial_output;
        label   = shortNotation
          ? formatGpioName_TX(false)
          : formatGpioName_serialTX(false);
        break;
      case NW005_KEY_PIN_CTS:
        purpose = PinSelectPurpose::Generic_input;
        label   = formatGpioName(
          label, gpio_direction::gpio_input, optional);
        break;
      case NW005_KEY_PIN_RTS:
        purpose = PinSelectPurpose::Generic_output;
        label   = formatGpioName(
          label, gpio_direction::gpio_output, optional);
        break;
      case NW005_KEY_PIN_RESET:
        purpose = PinSelectPurpose::Generic_output;
        label   = formatGpioName(
          label, gpio_direction::gpio_output, optional);
        break;
    }

  }
  return label;
}

} // namespace ppp
} // namespace net
} // namespace ESPEasy

#endif // ifdef USES_NW005
