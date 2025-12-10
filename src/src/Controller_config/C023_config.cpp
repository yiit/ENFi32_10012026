#include "../Controller_config/C023_config.h"

#ifdef USES_C023

# include "../Controller_struct/C023_data_struct.h"

# define C023_BAUDRATE_LABEL     "baudrate"

void C023_ConfigStruct::validate() {
  ZERO_TERMINATE(DeviceEUI);
  ZERO_TERMINATE(DeviceAddr);
  ZERO_TERMINATE(NetworkSessionKey);
  ZERO_TERMINATE(AppSessionKey);

  if ((baudrate < 2400) || (baudrate > 115200)) {
    reset();
  }
  if (LoRaWAN_Class > static_cast<uint8_t>(LoRaWANclass_e::C)) {
    // Set to default class A
    LoRaWAN_Class = static_cast<uint8_t>(LoRaWANclass_e::A);
  }
/*

  switch (frequencyplan) {
    case RN2xx3_datatypes::Freq_plan::SINGLE_CHANNEL_EU:
    case RN2xx3_datatypes::Freq_plan::TTN_EU:
    case RN2xx3_datatypes::Freq_plan::DEFAULT_EU:

      if ((rx2_freq < 867000000) || (rx2_freq > 870000000)) {
        rx2_freq = 0;
      }
      break;
    case RN2xx3_datatypes::Freq_plan::TTN_US:
      // FIXME TD-er: Need to find the ranges for US (and other regions)
      break;
    default:
      rx2_freq = 0;
      break;
  }
      */
}

void C023_ConfigStruct::reset() {
  ZERO_FILL(DeviceEUI);
  ZERO_FILL(DeviceAddr);
  ZERO_FILL(NetworkSessionKey);
  ZERO_FILL(AppSessionKey);
  baudrate      = 57600;
  rxpin         = -1;
  txpin         = -1;
  resetpin      = -1;
  sf            = 7;
//  frequencyplan = RN2xx3_datatypes::Freq_plan::TTN_EU;
  rx2_freq      = 0;
//  stackVersion  = RN2xx3_datatypes::TTN_stack_version::TTN_v3;
  joinmethod    = C023_USE_OTAA;
}

void C023_ConfigStruct::webform_load(C023_data_struct *C023_data) {
  validate();
  ESPEasySerialPort port = static_cast<ESPEasySerialPort>(serialPort);

  {
    addFormTextBox(F("Device EUI"), F("deveui"), DeviceEUI, C023_DEVICE_EUI_LEN - 1);
    String deveui_note = F("Leave empty to use HW DevEUI: ");

    if (C023_data != nullptr) {
      deveui_note += C023_data->hweui();
    }
    addFormNote(deveui_note, F("deveui_note"));
  }

  addFormTextBox(F("Device Addr"),         F("devaddr"), DeviceAddr,        C023_DEVICE_ADDR_LEN - 1);
  addFormTextBox(F("Network Session Key"), F("nskey"),   NetworkSessionKey, C023_NETWORK_SESSION_KEY_LEN - 1);
  addFormTextBox(F("App Session Key"),     F("appskey"), AppSessionKey,     C023_APP_SESSION_KEY_LEN - 1);

  {
    const __FlashStringHelper *options[] = { F("OTAA"),  F("ABP") };
    //const int values[]                   = { C023_USE_OTAA, C023_USE_ABP };
    FormSelectorOptions selector(NR_ELEMENTS(options), options);
    // Script to toggle OTAA/ABP fields visibility when changing selection. 
    selector.onChangeCall = F("joinChanged(this)"); 
    selector.addFormSelector(F("Activation Method"), F("joinmethod"), joinmethod);
                           
  }
  html_add_script(F("document.getElementById('joinmethod').onchange();"), false);

  addTableSeparator(F("Connection Configuration"), 2, 3);
  /*
  {
    const __FlashStringHelper *options[] = { F("SINGLE_CHANNEL_EU"), F("TTN_EU"), F("TTN_US"), F("DEFAULT_EU") };
    int values[]                         =
    {
      RN2xx3_datatypes::Freq_plan::SINGLE_CHANNEL_EU,
      RN2xx3_datatypes::Freq_plan::TTN_EU,
      RN2xx3_datatypes::Freq_plan::TTN_US,
      RN2xx3_datatypes::Freq_plan::DEFAULT_EU
    };
    const FormSelectorOptions selector( NR_ELEMENTS(options), options, values);
    selector.addFormSelector(F("Frequency Plan"), F("frequencyplan"), frequencyplan);
    addFormNumericBox(F("RX2 Frequency"), F("rx2freq"), rx2_freq, 0);
    addUnit(F("Hz"));
    addFormNote(F("0 = default, or else override default"));
  }
    */
  {
    const __FlashStringHelper *options[] = { F("A"), F("C") };
    constexpr int values[] {
      0,
      2
    };
    const FormSelectorOptions selector(NR_ELEMENTS(options), options, values);
    selector.addFormSelector(F("LoRaWAN Class"), F("loraclass"), LoRaWAN_Class);
  }

  addFormNumericBox(F("Spread Factor"), F("sf"), sf, 7, 12);
  addFormCheckBox(F("Adaptive Data Rate (ADR)"), F("adr"), adr);


  addTableSeparator(F("Serial Port Configuration"), 2, 3);

  serialHelper_webformLoad(port, rxpin, txpin, true);

  // Show serial port selection
  addFormPinSelect(PinSelectPurpose::Generic_input,  formatGpioName_serialRX(false), F("taskdevicepin1"), rxpin);
  addFormPinSelect(PinSelectPurpose::Generic_output, formatGpioName_serialTX(false), F("taskdevicepin2"), txpin);

  html_add_script(F("document.getElementById('serPort').onchange();"), false);

  addFormNumericBox(F("Baudrate"), F(C023_BAUDRATE_LABEL), baudrate, 2400, 115200);
  addUnit(F("baud"));
  addFormNote(F("Module default baudrate: 57600 bps"));

  // Optional reset pin RN2xx3
  addFormPinSelect(PinSelectPurpose::Generic_output, formatGpioName_output_optional(F("Reset")), F("taskdevicepin3"), resetpin);

    addTableSeparator(F("Downlink Messages"), 2, 3);

    {
    const __FlashStringHelper *options[] = { 
      F("PortNr in EventPar"),
      F("PortNr as 1st EventValue"),
      F("PortNr both EventPar & 1st EventValue")
    };
    constexpr int values[] {
      static_cast<int>(EventFormatStructure_e::PortNr_in_eventPar),
      static_cast<int>(EventFormatStructure_e::PortNr_as_first_eventvalue),
      static_cast<int>(EventFormatStructure_e::PortNr_both_eventPar_eventvalue)
    };
    const FormSelectorOptions selector(NR_ELEMENTS(options), options, values);
    selector.addFormSelector(F("Event Format"), F("eventformat"), eventFormat);
  }


  if (C023_data != nullptr) {
    addTableSeparator(F("Statistics"), 2, 3);
    // Some information on detected device
    addRowLabel(F("Voltage"));
    addHtmlFloat(static_cast<float>(C023_data->getVbat()) / 1000.0f, 3);

    addRowLabel(F("Device Addr"));
    addHtml(C023_data->getDevaddr());

    uint32_t dnctr, upctr;

    if (C023_data->getFrameCounters(dnctr, upctr)) {
      addRowLabel(F("Frame Counters (down/up)"));
      String values = String(dnctr);
      values += '/';
      values += upctr;
      addHtml(values);
    }

    addRowLabel(F("Last Command Error"));
    addHtml(C023_data->getLastError());

    addRowLabel(F("Sample Set Counter"));
    addHtmlInt(static_cast<uint32_t>(C023_data->getSampleSetCount()));

    addRowLabel(F("Data Rate"));
    addHtml(C023_data->getDataRate());

    /*
    {
      RN2xx3_status status = C023_data->getStatus();

      addRowLabel(F("Status RAW value"));
      addHtmlInt(status.getRawStatus());

      addRowLabel(F("Activation Status"));
      addEnabled(status.Joined);

      addRowLabel(F("Silent Immediately"));
      addHtmlInt(static_cast<uint32_t>(status.SilentImmediately ? 1 : 0));
    }
      */
  }
}

void C023_ConfigStruct::webform_save() {
  reset();
  String deveui  = webArg(F("deveui"));
  String devaddr = webArg(F("devaddr"));
  String nskey   = webArg(F("nskey"));
  String appskey = webArg(F("appskey"));

  strlcpy(DeviceEUI,         deveui.c_str(),  sizeof(DeviceEUI));
  strlcpy(DeviceAddr,        devaddr.c_str(), sizeof(DeviceAddr));
  strlcpy(NetworkSessionKey, nskey.c_str(),   sizeof(NetworkSessionKey));
  strlcpy(AppSessionKey,     appskey.c_str(), sizeof(AppSessionKey));
  baudrate      = getFormItemInt(F(C023_BAUDRATE_LABEL), baudrate);
  rxpin         = getFormItemInt(F("taskdevicepin1"), rxpin);
  txpin         = getFormItemInt(F("taskdevicepin2"), txpin);
  resetpin      = getFormItemInt(F("taskdevicepin3"), resetpin);
  sf            = getFormItemInt(F("sf"), sf);
  eventFormat   = getFormItemInt(F("eventformat"), eventFormat);
  rx2_freq      = getFormItemInt(F("rx2freq"), rx2_freq);
  joinmethod    = getFormItemInt(F("joinmethod"), joinmethod);
  LoRaWAN_Class = getFormItemInt(F("loraclass"), LoRaWAN_Class);
  adr           = isFormItemChecked(F("adr"));
  serialHelper_webformSave(serialPort, rxpin, txpin);
}

#endif // ifdef USES_C023
