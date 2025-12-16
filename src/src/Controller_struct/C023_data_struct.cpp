#include "../Controller_struct/C023_data_struct.h"

#ifdef USES_C023


# include <ESPeasySerial.h>


C023_data_struct::C023_data_struct() :
  _easySerial(nullptr) {}

C023_data_struct::~C023_data_struct() {
  if (_easySerial != nullptr) {
    _easySerial->end();
    delete _easySerial;
    _easySerial = nullptr;
  }
}

void C023_data_struct::reset() {
  if (_easySerial != nullptr) {
    _easySerial->end();
    delete _easySerial;
    _easySerial = nullptr;
  }
  _cachedValues.clear();
  _queuedQueries.clear();
  clearQueryPending();
}

bool C023_data_struct::init(
  const C023_ConfigStruct& config,
  taskIndex_t              sampleSet_Initiator)
{
  const uint8_t port      = config.serialPort;
  const int8_t  serial_rx = config.rxpin;
  const int8_t  serial_tx = config.txpin;
  unsigned long baudrate  = config.baudrate;
  bool   joinIsOTAA       = (config.joinmethod == C023_USE_OTAA);
  int8_t reset_pin        = config.resetpin;

  _eventFormatStructure = config.getEventFormat();

  if ((serial_rx < 0) || (serial_tx < 0)) {
    // Both pins are needed, or else no serial possible
    return false;
  }

  // FIXME TD-er: Prevent unneeded OTAA joins.
  // See: https://www.thethingsnetwork.org/forum/t/how-often-should-a-node-do-an-otaa-join-and-is-otaa-better-than-abp/11192/47?u=td-er


  sampleSetInitiator = sampleSet_Initiator;

  if (isInitialized()) {
    // Check to see if serial parameters have changed.
    bool notChanged = true;
    notChanged &= _easySerial->getRxPin() == serial_rx;
    notChanged &= _easySerial->getTxPin() == serial_tx;
    notChanged &= _easySerial->getBaudRate() == static_cast<int>(baudrate);

    if (notChanged) { return true; }
  }
  reset();
  _resetPin   = reset_pin;
  _baudrate   = baudrate;
  _isClassA   = config.getClass() == LoRa_Helper::LoRaWANclass_e::A;
  _dr         = config.getDR();
  _loraModule = static_cast<C023_AT_commands::LoRaModule_e>(config.LoRa_module);

  // FIXME TD-er: Make force SW serial a proper setting.
  if (_easySerial != nullptr) {
    delete _easySerial;
  }

  // When calling "AT+CFG", we may get quite a lot of data at once at a relatively low baud rate.
  // This requires quite a lot of calls to read it, so it is much more likely to have some other call inbetween taking way longer than 20
  // msec and thus we will miss some data

  _easySerial = new (std::nothrow) ESPeasySerial(static_cast<ESPEasySerialPort>(port), serial_rx, serial_tx, false, 1024);

  if (_easySerial != nullptr) {
    _easySerial->begin(baudrate);

    _easySerial->println(F("ATZ")); // Reset LoRa
    delay(1000);

    if (_loraModule == C023_AT_commands::LoRaModule_e::RAK_3172) {
      _easySerial->println(F("AT+NWM=1")); // Set to LoRaWAN mode (M5-LoRaWAN-RAK)
    }
    setDR(_dr);                            // TODO TD-er: Must this be called after join?

    if (_loraModule == C023_AT_commands::LoRaModule_e::Dragino_LA66) {
      _easySerial->println(F("AT+CFG"));   // AT+CFG: Print all configurations
    } else {
      for (size_t i = 0; i != static_cast<size_t>(C023_AT_commands::AT_cmd::Unknown); ++i) {
        const C023_AT_commands::AT_cmd cmd = static_cast<C023_AT_commands::AT_cmd>(i);
        sendQuery(cmd);
      }
    }
  }
  return isInitialized();
}

bool C023_data_struct::hasJoined() { return getInt(C023_AT_commands::AT_cmd::NJS, 0) != 0; }

bool C023_data_struct::useOTAA()   { return getInt(C023_AT_commands::AT_cmd::NJM, 1) == 1; }

bool C023_data_struct::command_finished() const {
  return true; // myLora->command_finished();
}

bool C023_data_struct::txUncnfBytes(const uint8_t *data, uint8_t size, uint8_t port) {
  bool res = true; // myLora->txBytes(data, size, port) != RN2xx3_datatypes::TX_return_type::TX_FAIL;

  return res;
}

bool C023_data_struct::txHexBytes(const String& data, uint8_t port) {
  if (!isInitialized()) { return false; }
  bool   res      = true;
  String sendData = data;
  sendData.replace(F(" "), F(""));
  sendData.trim();

  // "AT+SENDB=0,2,4,11223344"
  // confirm status,Fport,payload length,payload(HEX)

  _easySerial->println(
    strformat(
      F("AT+SENDB=%d,%d,%d,%s"),
      0,                     // confirm status
      port,                  // Fport
      sendData.length() / 2, // payload length
      sendData.c_str()));    // payload(HEX)

  // TODO TD-er: Must wait for either "OK" or "AT_BUSY_ERROR"
  // This 'busy error' may occur in case the previous send is
  // not completed, because of the duty cycle restriction,
  // or because RX windows are not completed

  return res;
}

bool C023_data_struct::txUncnf(const String& data, uint8_t port) {
  bool res = true; // myLora->tx(data, port) != RN2xx3_datatypes::TX_return_type::TX_FAIL;

  return res;
}

bool C023_data_struct::setDR(LoRa_Helper::LoRaWAN_DR dr)
{
  if (!isInitialized()) { return false; }

  if (dr == LoRa_Helper::LoRaWAN_DR::ADR) {
    _easySerial->println(F("AT+ADR=1"));
  } else {
    _easySerial->println(F("AT+ADR=0"));
    _easySerial->println(concat(F("AT+DR="), static_cast<int>(dr)));
  }
  get(C023_AT_commands::AT_cmd::ADR);
  get(C023_AT_commands::AT_cmd::DR);
  return true;
}

bool C023_data_struct::initOTAA(const String& AppEUI, const String& AppKey, const String& DevEUI) {
  //  if (myLora == nullptr) { return false; }
  bool success = true; // myLora->initOTAA(AppEUI, AppKey, DevEUI);

  return success;
}

bool C023_data_struct::initABP(const String& addr, const String& AppSKey, const String& NwkSKey) {
  //  if (myLora == nullptr) { return false; }
  bool success = true; // myLora->initABP(addr, AppSKey, NwkSKey);

  return success;
}

String C023_data_struct::sendRawCommand(const String& command) {
  if (!isInitialized()) { return EMPTY_STRING; }

  if (loglevelActiveFor(LOG_LEVEL_INFO)) {
    String log = F("sendRawCommand: ");
    log += command;
    addLogMove(LOG_LEVEL_INFO, log);
  }
  String res; // = myLora->sendRawCommand(command);
  return res;
}

int    C023_data_struct::getVbat() { return getInt(C023_AT_commands::AT_cmd::BAT, -1); }

String C023_data_struct::peekLastError() {
  if (!isInitialized()) { return EMPTY_STRING; }
  return EMPTY_STRING; // myLora->peekLastError();
}

String C023_data_struct::getLastError() {
  if (!isInitialized()) { return EMPTY_STRING; }
  return EMPTY_STRING; // myLora->getLastError();
}

LoRa_Helper::LoRaWAN_DR C023_data_struct::getDataRate()
{
  const int dr_int = getInt(C023_AT_commands::AT_cmd::DR, -1);

  if (dr_int == -1) {
    return LoRa_Helper::LoRaWAN_DR::ADR;
  }
  return static_cast<LoRa_Helper::LoRaWAN_DR>(dr_int);
}

String C023_data_struct::getDataRate_str() {
  const int dr_int = getInt(C023_AT_commands::AT_cmd::DR, -1);

  if (dr_int == -1) { return F("-"); }
  LoRa_Helper::LoRaWAN_DR dr =
    static_cast<LoRa_Helper::LoRaWAN_DR>(dr_int);
  return strformat(F("%d: %s"), dr_int, FsP(LoRa_Helper::toString(dr)));
}

int      C023_data_struct::getRSSI() { return getInt(C023_AT_commands::AT_cmd::RSSI, 0); }

uint32_t C023_data_struct::getRawStatus() {
  if (!isInitialized()) { return 0; }
  return 0; // myLora->getStatus().getRawStatus();
}

bool C023_data_struct::getFrameCounters(uint32_t& dnctr, uint32_t& upctr) {
  if (!isInitialized()) { return false; }
  dnctr = getInt(C023_AT_commands::AT_cmd::FCD, 0);
  upctr = getInt(C023_AT_commands::AT_cmd::FCU, 0);
  return true;
}

bool C023_data_struct::setFrameCounters(uint32_t dnctr, uint32_t upctr) {
  if (!isInitialized()) { return false; }
  bool res = true; // myLora->setFrameCounters(dnctr, upctr);

  return res;
}

// Cached data, only changing occasionally.

String  C023_data_struct::getDevaddr() { return get(C023_AT_commands::AT_cmd::DADDR); }

String  C023_data_struct::hweui()      { return get(C023_AT_commands::AT_cmd::DEUI); }

String  C023_data_struct::sysver()     { return get(C023_AT_commands::AT_cmd::VER); }

uint8_t C023_data_struct::getSampleSetCount() const {
  return sampleSetCounter;
}

uint8_t C023_data_struct::getSampleSetCount(taskIndex_t taskIndex) {
  if (sampleSetInitiator == taskIndex)
  {
    ++sampleSetCounter;
  }
  return sampleSetCounter;
}

float C023_data_struct::getLoRaAirTime(uint8_t pl) {
  if (isInitialized()) {
    return LoRa_Helper::getLoRaAirTime(pl, getDataRate());
  }
  return -1.0f;
}

void C023_data_struct::async_loop() {
  if (isInitialized()) {
    /*
       rn2xx3_handler::RN_state state = myLora->async_loop();

       if (rn2xx3_handler::RN_state::must_perform_init == state) {
       if (myLora->get_busy_count() > 10) {
        if (validGpio(_resetPin)) {
          pinMode(_resetPin, OUTPUT);
          digitalWrite(_resetPin, LOW);
          delay(50);
          digitalWrite(_resetPin, HIGH);
          delay(200);
        }
        autobaud_success = false;

        //          triggerAutobaud();
       }
       }
     */
    while (_easySerial->available()) {
      const int ret = _easySerial->read();

      if (ret >= 0) {
        const char c = static_cast<char>(ret);

        switch (c)
        {
          case '\n':
          case '\r':
          {
            // End of line
            if (!_fromLA66.isEmpty()) {
              addLog(LOG_LEVEL_INFO, concat(F("LoRa recv: "), _fromLA66));

              // TODO TD-er: Process received data
              processReceived(_fromLA66);
            }

            _fromLA66.clear();
            break;
          }
          default:
            _fromLA66 += c;
            break;
        }
      }
    }

    sendNextQueuedQuery();
  }
}

bool C023_data_struct::writeCachedValues(KeyValueWriter*writer, C023_AT_commands::AT_cmd start, C023_AT_commands::AT_cmd end)
{
  if (writer == nullptr) { return false; }

  for (size_t i = static_cast<size_t>(start); i < static_cast<size_t>(end); ++i) {
    const C023_AT_commands::AT_cmd cmd = static_cast<C023_AT_commands::AT_cmd>(i);

    const String value = get(cmd);

    if (!value.isEmpty()) {
      auto kv = C023_AT_commands::getKeyValue(cmd, _loraModule, value, true /*!writer->dataOnlyOutput()*/);
      writer->write(kv);
    }
  }
  return true;
}

String C023_data_struct::get(C023_AT_commands::AT_cmd at_cmd, uint32_t& lastChange)
{
  lastChange = 0;

  if (isInitialized() && (at_cmd != C023_AT_commands::AT_cmd::Unknown)) {
    auto it = _cachedValues.find(static_cast<size_t>(at_cmd));

    if (it != _cachedValues.end()) {
      if (C023_AT_commands::isVolatileValue(at_cmd) && it->second.expired()) {
        sendQuery(at_cmd, true);
      }
      lastChange = it->second.lastChange;
      return it->second.value;
    }
    sendQuery(at_cmd);
  }
  return EMPTY_STRING;

}

int C023_data_struct::getInt(C023_AT_commands::AT_cmd at_cmd,
                             int                      errorvalue,
                             uint32_t               & lastChange)
{
  String value = get(at_cmd, lastChange);

  if (value.isEmpty()) {
    return errorvalue;
  }
  return value.toInt();
}

float C023_data_struct::getFloat(C023_AT_commands::AT_cmd at_cmd,
                                 float                    errorvalue,
                                 uint32_t               & lastChange)
{
  String value = get(at_cmd, lastChange);

  if (value.isEmpty()) {
    return errorvalue;
  }
  float res{};

  if (validFloatFromString(value, res)) { return res; }
  return errorvalue;
}

String C023_data_struct::get(C023_AT_commands::AT_cmd at_cmd)
{
  uint32_t lastChange{};

  return get(at_cmd, lastChange);
}

int C023_data_struct::getInt(C023_AT_commands::AT_cmd at_cmd, int errorvalue)
{
  uint32_t lastChange{};

  return getInt(at_cmd, errorvalue, lastChange);
}

float C023_data_struct::getFloat(C023_AT_commands::AT_cmd at_cmd,
                                 float                    errorvalue)
{
  uint32_t lastChange{};

  return getFloat(at_cmd, errorvalue, lastChange);
}

bool C023_data_struct::processReceived(const String& receivedData)
{
  String value;
  const C023_AT_commands::AT_cmd at_cmd = C023_AT_commands::decode(receivedData, value);

  if (at_cmd == C023_AT_commands::AT_cmd::Unknown) {
    switch (_loraModule)
    {
      case C023_AT_commands::LoRaModule_e::Dragino_LA66:
        processReceived_Dragino_LA66(receivedData, at_cmd);
        break;
      case C023_AT_commands::LoRaModule_e::RAK_3172:
        processReceived_RAK_3172(receivedData, at_cmd);
        break;

      case C023_AT_commands::LoRaModule_e::MAX_TYPE:
        break;
    }
    return false;
  }
  cacheValue(at_cmd, std::move(value));

  if (_queryPending == at_cmd) {
    clearQueryPending();
    sendNextQueuedQuery();
  }

  return true;
}

bool C023_data_struct::processReceived_Dragino_LA66(const String& receivedData, C023_AT_commands::AT_cmd at_cmd)
{
  if (receivedData.equals(F("txDone"))) {
    sendQuery(C023_AT_commands::AT_cmd::FCU);
  } else if (receivedData.equals(F("rxDone"))) {
    sendQuery(C023_AT_commands::AT_cmd::FCD);
    sendQuery(C023_AT_commands::AT_cmd::SNR);
  } else if (receivedData.indexOf(F("ADR Message")) != -1) {
    sendQuery(C023_AT_commands::AT_cmd::ADR);
    sendQuery(C023_AT_commands::AT_cmd::DR);
  } else if (receivedData.equals(F("JOINED"))) {
    _easySerial->println(concat(F("AT+CLASS="), _isClassA ? 'A' : 'C'));

    // Enable Sync system time via LoRaWAN MAC Command (DeviceTimeReq), LoRaWAN server must support v1.0.3 protocol to reply this command.
    _easySerial->println(F("AT+SYNCMOD=1"));

    _easySerial->println(F("AT+CFG")); // AT+CFG: Print all configurations
    //      sendQuery(C023_AT_commands::AT_cmd::NJM);
    //      sendQuery(C023_AT_commands::AT_cmd::NJS);
    eventQueue.add(F("LoRa#joined"));
  } else if (receivedData.equals(F("rxTimeout"))) {
    // Just skip this one, no data received
  } else if (receivedData.startsWith(F("Rssi"))) {
    cacheValue(C023_AT_commands::AT_cmd::RSSI, getValueFromReceivedData(receivedData));
  } else if (receivedData.indexOf(F("DevEui")) != -1) {
    cacheValue(C023_AT_commands::AT_cmd::DEUI, getValueFromReceivedData(receivedData));
  } else if (receivedData.indexOf(F("AT+RECVB=?")) != -1) {
    sendQuery(C023_AT_commands::AT_cmd::RECVB, true);
  } else if (receivedData.indexOf(F("AT+RECV=?")) != -1) {
    sendQuery(C023_AT_commands::AT_cmd::RECV, true);
  } else if (receivedData.startsWith(F("*****")) ||
             receivedData.startsWith(F("TX on")) ||
             receivedData.startsWith(F("RX on")))
  {
    // Ignore these lines for now.
    // Maybe those "***** UpLinkCounter= 51 *****" could be parsed
    // also:
    //   TX on freq 867.100 MHz at DR 5
    //   RX on freq 869.525 MHz at DR 3
  } else if (receivedData.indexOf(F("Tx events")) != -1) {
    // Ignore replies like these:
    //   Stop Tx events,Please wait for the erase to complete
    //   Start Tx events
    // TODO TD-er: Maybe use these to ignore/pause parsing possible pending queries?
  }

  else if (
    receivedData.equals(F("AT_ERROR")) ||               // Generic error
    receivedData.equals(F("AT_PARAM_ERROR")) ||         // A parameter of the command is wrong
    receivedData.equals(F("AT_BUSY_ERROR")) ||          // the LoRa® network is busy, so the command could not completed
    receivedData.equals(F("AT_TEST_PARAM_OVERFLOW")) || // the parameter is too long
    receivedData.equals(F("AT_NO_NETWORK_JOINED")) ||   // the LoRa® network has not been joined yet
    receivedData.equals(F("AT_RX_ERROR")))              // error detection during the reception of the command
  {
    if (!queryPending()) {
      addLog(LOG_LEVEL_ERROR, strformat(
               F("LoRa   : %s"),
               receivedData.c_str()));
    } else {
      addLog(LOG_LEVEL_ERROR, strformat(
               F("LoRa   : %s while processing %s"),
               receivedData.c_str(),
               C023_AT_commands::toString(_queryPending, _loraModule).c_str()));
      clearQueryPending();
    }
  }

  else if (receivedData.equals(F("OK"))) {
    // Just ignore
  } else {
    processPendingQuery(receivedData);
  }

  return true;
}

bool C023_data_struct::processReceived_RAK_3172(const String& receivedData, C023_AT_commands::AT_cmd at_cmd)
{
  // See: https://docs.rakwireless.com/product-categories/software-apis-and-libraries/rui3/at-command-manual/#asynchronous-events
  if (receivedData.indexOf(F("+EVT:TX_DONE")) != -1) {
    sendQuery(C023_AT_commands::AT_cmd::FCU);
  } else if (receivedData.indexOf(F("+EVT:RX_")) != -1) {
    // TODO TD-er: Must parse received data
    // Example for class B/C:
    //  RX_B:-47:3:UNICAST:2:4321 -47 is RSSI, 3 is SNR, Unicast for B / Multicast for C, 2 is Fport, 4321 is payload.
    // Class A:
    //  RX_1:-70:8:UNICAST:1:1234 -70 is RSSI, 8 is SNR, 1 is Fport, 1234 is payload.
    sendQuery(C023_AT_commands::AT_cmd::FCD);
    sendQuery(C023_AT_commands::AT_cmd::SNR);
  } else if (receivedData.indexOf(F("ADR Message")) != -1) {
    sendQuery(C023_AT_commands::AT_cmd::ADR);
    sendQuery(C023_AT_commands::AT_cmd::DR);
  } else if (receivedData.indexOf(F("+EVT:JOINED")) != -1) {
    _easySerial->println(concat(F("AT+CLASS="), _isClassA ? 'A' : 'C'));
    sendQuery(C023_AT_commands::AT_cmd::NJM);
    sendQuery(C023_AT_commands::AT_cmd::NJS);
    eventQueue.add(F("LoRa#joined"));
  } else if (receivedData.equals(F("rxTimeout"))) {
    // Just skip this one, no data received
  } else if (receivedData.startsWith(F("Rssi"))) {
    cacheValue(C023_AT_commands::AT_cmd::RSSI, getValueFromReceivedData(receivedData));
  } else if (receivedData.indexOf(F("DevEui")) != -1) {
    cacheValue(C023_AT_commands::AT_cmd::DEUI, getValueFromReceivedData(receivedData));
  } else if (receivedData.indexOf(F("AT+BAT=")) != -1) {
    const float vbat    = getValueFromReceivedData(receivedData).toFloat();
    const int   vbat_mv = vbat * 1000.0f;
    cacheValue(C023_AT_commands::AT_cmd::DEUI, String(vbat_mv));
  } else if (receivedData.indexOf(F("AT+RECVB=?")) != -1) {
    sendQuery(C023_AT_commands::AT_cmd::RECVB, true);
  } else if (receivedData.indexOf(F("AT+RECV=?")) != -1) {
    sendQuery(C023_AT_commands::AT_cmd::RECV, true);
  } else if (receivedData.startsWith(F("*****")) ||
             receivedData.startsWith(F("TX on")) ||
             receivedData.startsWith(F("RX on")))
  {
    // Ignore these lines for now.
    // Maybe those "***** UpLinkCounter= 51 *****" could be parsed
    // also:
    //   TX on freq 867.100 MHz at DR 5
    //   RX on freq 869.525 MHz at DR 3
  } else if (receivedData.indexOf(F("Tx events")) != -1) {
    // Ignore replies like these:
    //   Stop Tx events,Please wait for the erase to complete
    //   Start Tx events
    // TODO TD-er: Maybe use these to ignore/pause parsing possible pending queries?
  }

  else if (
    receivedData.equals(F("AT_ERROR")) ||               // Generic error
    receivedData.equals(F("AT_COMMAND_NOT_FOUND")) ||   // Unknown AT command
    receivedData.equals(F("AT_PARAM_ERROR")) ||         // A parameter of the command is wrong
    receivedData.equals(F("AT_BUSY_ERROR")) ||          // the LoRa® network is busy, so the command could not completed
    receivedData.equals(F("AT_TEST_PARAM_OVERFLOW")) || // the parameter is too long
    receivedData.equals(F("AT_NO_NETWORK_JOINED")) ||   // the LoRa® network has not been joined yet
    receivedData.equals(F("AT_RX_ERROR")))              // error detection during the reception of the command
  {
    if (!queryPending()) {
      addLog(LOG_LEVEL_ERROR, strformat(
               F("LoRa   : %s"),
               receivedData.c_str()));
    } else {
      addLog(LOG_LEVEL_ERROR, strformat(
               F("LoRa   : %s while processing %s"),
               receivedData.c_str(),
               C023_AT_commands::toString(_queryPending, _loraModule).c_str()));
      clearQueryPending();
    }
  }

  else if (receivedData.equals(F("OK"))) {
    // Just ignore
  } else {
    processPendingQuery(receivedData);
  }

  return true;
}

bool C023_data_struct::processPendingQuery(const String& receivedData)
{
  if (!queryPending()) {
    sendNextQueuedQuery();
    return false;
  }

  if ((_queryPending == C023_AT_commands::AT_cmd::RECVB) ||
      (_queryPending == C023_AT_commands::AT_cmd::RECV)) {
    const bool hexEncoded = _queryPending == C023_AT_commands::AT_cmd::RECVB;
    int port{};
    const String value = getValueFromReceivedBinaryData(
      port, receivedData, hexEncoded);

    if ((port > 0) && (value.length() != 0)) {
      switch (_eventFormatStructure)
      {
        case LoRa_Helper::DownlinkEventFormat_e::PortNr_in_eventPar:
          eventQueue.addMove(strformat(F("LoRa#received%d=%s"), port, value.c_str()));
          break;
        case LoRa_Helper::DownlinkEventFormat_e::PortNr_as_first_eventvalue:
          eventQueue.addMove(strformat(F("LoRa#received=%d,%s"), port, value.c_str()));
          break;
        case LoRa_Helper::DownlinkEventFormat_e::PortNr_both_eventPar_eventvalue:
          eventQueue.addMove(strformat(F("LoRa#received%d=%d,%s"), port, port, value.c_str()));
          break;
      }
    }

    cacheValue(_queryPending, strformat(
                 F("%s -> %d : %s"), receivedData.c_str(), port, value.c_str()));
  } else {
    cacheValue(_queryPending, receivedData);
  }
  addLog(LOG_LEVEL_INFO, strformat(
           F("LoRa : Process Query: %s -> %s"),
           C023_AT_commands::toString(_queryPending, _loraModule).c_str(),
           receivedData.c_str()));

  clearQueryPending();
  sendNextQueuedQuery();
  return true;

}

void C023_data_struct::sendQuery(C023_AT_commands::AT_cmd at_cmd, bool prioritize)
{
  if (_easySerial) {
    if (!C023_AT_commands::supported(at_cmd, _loraModule)) {
      return;
    }

    if (prioritize) {
      if (_queuedQueries.front() != static_cast<size_t>(at_cmd)) {
        _queuedQueries.push_front(static_cast<size_t>(at_cmd));
      }
      async_loop();
    } else {
      auto it = _queuedQueries.remove(static_cast<size_t>(at_cmd));
      _queuedQueries.push_back(static_cast<size_t>(at_cmd));
    }

    if (loglevelActiveFor(LOG_LEVEL_INFO)) {
      const String query = concat(C023_AT_commands::toString(at_cmd, _loraModule), F("=?"));
      addLog(LOG_LEVEL_INFO, concat(F("LoRa : Add to queue: "), query));
    }

    if (prioritize) {
      delay(10);
      async_loop();
    }
  }
}

void C023_data_struct::sendNextQueuedQuery()
{
  if (!queryPending() && !_queuedQueries.empty()) {
    _queryPending = static_cast<C023_AT_commands::AT_cmd>(_queuedQueries.front());
    _queuedQueries.pop_front();
    const String query = concat(C023_AT_commands::toString(_queryPending, _loraModule), F("=?"));

    addLog(LOG_LEVEL_INFO, concat(F("LoRa : Queried "), query));

    _easySerial->println(query);
    _querySent = millis();
  }
}

void C023_data_struct::cacheValue(C023_AT_commands::AT_cmd at_cmd, const String& value)
{
  String tmp(value);

  cacheValue(at_cmd, std::move(tmp));
}

void C023_data_struct::cacheValue(C023_AT_commands::AT_cmd at_cmd, String&& value)
{
  if (value.isEmpty()) { return; }
  const size_t key = static_cast<size_t>(at_cmd);
  auto it          = _cachedValues.find(key);

  if (it != _cachedValues.end()) {
    it->second.set(value);
  } else {
    _cachedValues.emplace(key, std::move(value));
  }
}

String C023_data_struct::getValueFromReceivedData(const String& receivedData)
{
  const int pos = receivedData.indexOf('=');

  if (pos == -1) { return EMPTY_STRING; }
  String res = receivedData.substring(pos + 1);
  res.trim();
  return res;
}

String C023_data_struct::getValueFromReceivedBinaryData(int& port, const String& receivedData, bool hexEncoded)
{
  port = -1;
  int pos = receivedData.indexOf(':');

  if (pos == -1) { return EMPTY_STRING; }
  port = receivedData.substring(0, pos).toInt();

  addLog(LOG_LEVEL_INFO, concat(
           F("LoRa fromHex: "), receivedData.substring(pos + 1)));

  if (hexEncoded) {
    return stringFromHexArray(receivedData.substring(pos + 1));
  }
  return receivedData.substring(pos + 1);
}

#endif // ifdef USES_C023
