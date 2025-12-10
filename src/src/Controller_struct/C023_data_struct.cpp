#include "../Controller_struct/C023_data_struct.h"

#ifdef USES_C023


# include <ESPeasySerial.h>


C023_data_struct::C023_data_struct() :
  C023_easySerial(nullptr) {}

C023_data_struct::~C023_data_struct() {
  if (C023_easySerial != nullptr) {
    C023_easySerial->end();
    delete C023_easySerial;
    C023_easySerial = nullptr;
  }
}

void C023_data_struct::reset() {
  if (C023_easySerial != nullptr) {
    C023_easySerial->end();
    delete C023_easySerial;
    C023_easySerial = nullptr;
  }
  _cachedValues.clear();
  _queuedQueries.clear();
  _queryPending = C023_AT_commands::AT_cmd::Unknown;
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
    notChanged &= C023_easySerial->getRxPin() == serial_rx;
    notChanged &= C023_easySerial->getTxPin() == serial_tx;
    notChanged &= C023_easySerial->getBaudRate() == static_cast<int>(baudrate);

    if (notChanged) { return true; }
  }
  reset();
  _resetPin = reset_pin;
  _baudrate = baudrate;

  // FIXME TD-er: Make force SW serial a proper setting.
  if (C023_easySerial != nullptr) {
    delete C023_easySerial;
  }

  C023_easySerial = new (std::nothrow) ESPeasySerial(static_cast<ESPEasySerialPort>(port), serial_rx, serial_tx, false, 64);

  if (C023_easySerial != nullptr) {
    C023_easySerial->begin(baudrate);

    const bool isClassA = config.getClass() == C023_ConfigStruct::LoRaWANclass_e::A;
    C023_easySerial->println(F("ATZ")); // Reset LA66
    delay(1000);

    C023_easySerial->println(concat(F("AT+CLASS="), isClassA ? 'A' : 'C'));
    delay(1000);

    C023_easySerial->println(F("AT+CFG")); // AT+CFG: Print all configurations
  }
  return isInitialized();
}

bool C023_data_struct::hasJoined() {
  if (!isInitialized()) { return false; }
  return getInt(C023_AT_commands::AT_cmd::NJS, 0) != 0;
}

bool C023_data_struct::useOTAA() {
  if (!isInitialized()) { return true; }
  return getInt(C023_AT_commands::AT_cmd::NJM, 1) == 1;
}

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

  C023_easySerial->println(
    strformat(
      F("AT+SENDB=%d,%d,%d,%s"),
      0,                     // confirm status
      port,                  // Fport
      sendData.length() / 2, // payload length
      sendData.c_str()));    // payload(HEX)

  return res;
}

bool C023_data_struct::txUncnf(const String& data, uint8_t port) {
  bool res = true; // myLora->tx(data, port) != RN2xx3_datatypes::TX_return_type::TX_FAIL;

  return res;
}

bool C023_data_struct::setSF(uint8_t sf) {
  if (!isInitialized()) { return false; }
  bool res = true; // myLora->setSF(sf);
  return res;
}

bool C023_data_struct::setAdaptiveDataRate(bool enabled) {
  if (!isInitialized()) { return false; }
  bool res = true; // myLora->setAdaptiveDataRate(enabled);
  return res;
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

String   C023_data_struct::getDataRate() { return get(C023_AT_commands::AT_cmd::DR); }

int      C023_data_struct::getRSSI()     { return getInt(C023_AT_commands::AT_cmd::RSSI, 0); }

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

float C023_data_struct::getLoRaAirTime(uint8_t pl) const {
  if (isInitialized()) {
    return 0.0f; // myLora->getLoRaAirTime(pl + 13); // We have a LoRaWAN header of 13 bytes.
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
    while (C023_easySerial->available()) {
      const int ret = C023_easySerial->read();

      if (ret >= 0) {
        const char c = static_cast<char>(ret);

        switch (c)
        {
          case '\n':
          case '\r':
          {
            // End of line
            if (!_fromLA66.isEmpty()) {
              addLog(LOG_LEVEL_INFO, concat(F("LA66 recv: "), _fromLA66));

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

bool C023_data_struct::writeCachedValues(KeyValueWriter*writer)
{
  if (writer == nullptr) { return false; }

  for (size_t i = 0; i < static_cast<size_t>(C023_AT_commands::AT_cmd::Unknown); ++i) {
    const C023_AT_commands::AT_cmd cmd = static_cast<C023_AT_commands::AT_cmd>(i);

    String value = get(cmd);

    if (!value.isEmpty()) {
      auto kv = C023_AT_commands::getKeyValue(cmd, value, true /*!writer->dataOnlyOutput()*/);
      writer->write(kv);
    }
  }
  return true;
}

String C023_data_struct::get(C023_AT_commands::AT_cmd at_cmd)
{
  if (isInitialized() && (at_cmd != C023_AT_commands::AT_cmd::Unknown)) {
    auto it = _cachedValues.find(static_cast<size_t>(at_cmd));

    if (it != _cachedValues.end()) {
      if (!it->second.expired()) {
        return it->second.value;
      }
      _cachedValues.erase(it);
    }
    sendQuery(at_cmd);
  }
  return EMPTY_STRING;
}

int C023_data_struct::getInt(C023_AT_commands::AT_cmd at_cmd, int errorvalue)
{
  String value = get(at_cmd);

  if (value.isEmpty()) {
    return errorvalue;
  }
  return value.toInt();
}

bool C023_data_struct::processReceived(const String& receivedData)
{
  String value;
  const C023_AT_commands::AT_cmd at_cmd = C023_AT_commands::decode(receivedData, value);

  if (at_cmd == C023_AT_commands::AT_cmd::Unknown) {
    if (receivedData.equals(F("txDone"))) {
      sendQuery(C023_AT_commands::AT_cmd::FCU);
    } else if (receivedData.equals(F("rxDone"))) {
      sendQuery(C023_AT_commands::AT_cmd::FCD);
      sendQuery(C023_AT_commands::AT_cmd::SNR);
    } else if (receivedData.equals(F("ADR Message"))) {
      sendQuery(C023_AT_commands::AT_cmd::ADR);
      sendQuery(C023_AT_commands::AT_cmd::DR, true);
    } else if (receivedData.equals(F("JOINED"))) {
      C023_easySerial->println(F("AT+CFG")); // AT+CFG: Print all configurations
      //      sendQuery(C023_AT_commands::AT_cmd::NJM);
      //      sendQuery(C023_AT_commands::AT_cmd::NJS);
      eventQueue.add(F("LA66#joined"));
    } else if (receivedData.equals(F("rxTimeout"))) {
      // Just skip this one, no data received
    } else if (receivedData.startsWith(F("Rssi"))) {
      cacheValue(C023_AT_commands::AT_cmd::RSSI, getValueFromReceivedData(receivedData));
    } else if (receivedData.indexOf(F("DevEui")) != -1) {
      cacheValue(C023_AT_commands::AT_cmd::DEUI, getValueFromReceivedData(receivedData));
    } else if (receivedData.indexOf(F("AT+RECVB=?")) != -1) {
      sendQuery(C023_AT_commands::AT_cmd::RECVB, true);
    } else if (receivedData.startsWith(F("*****")) ||
               receivedData.startsWith(F("TX on")) ||
               receivedData.startsWith(F("RX on")))
    {
      // Ignore these lines for now.
      // Maybe those "***** UpLinkCounter= 51 *****" could be parsed
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
                 F("LA66   : %s"),
                 receivedData.c_str()));
      } else {
        addLog(LOG_LEVEL_ERROR, strformat(
                 F("LA66   : %s while processing %s"),
                 receivedData.c_str(),
                 C023_AT_commands::toString(_queryPending).c_str()));
        _queryPending = C023_AT_commands::AT_cmd::Unknown;
      }
    }

    else if (receivedData.equals(F("OK"))) {
      // Just ignore
    } else {
      processPendingQuery(receivedData);
    }

    return false;
  }
  cacheValue(at_cmd, std::move(value));

  return true;
}

bool C023_data_struct::processPendingQuery(const String& receivedData)
{
  if (!queryPending()) {
    return false;
  }

  if (_queryPending == C023_AT_commands::AT_cmd::RECVB) {
    int port{};
    String value = getValueFromReceivedBinaryData(port, receivedData);

    if ((port > 0) && (value.length() != 0)) {
      switch (_eventFormatStructure)
      {
        case C023_ConfigStruct::EventFormatStructure_e::PortNr_in_eventPar:
          eventQueue.addMove(strformat(F("LA66#received%d=%s"), port, value.c_str()));
          break;
        case C023_ConfigStruct::EventFormatStructure_e::PortNr_as_first_eventvalue:
          eventQueue.addMove(strformat(F("LA66#received=%d,%s"), port, value.c_str()));
          break;
        case C023_ConfigStruct::EventFormatStructure_e::PortNr_both_eventPar_eventvalue:
          eventQueue.addMove(strformat(F("LA66#received%d=%d,%s"), port, port, value.c_str()));
          break;
      }
    }

    cacheValue(_queryPending, strformat(
                 F("%s -> %d : %s"), receivedData.c_str(), port, value.c_str()));
  } else {
    cacheValue(_queryPending, receivedData);
  }
  addLog(LOG_LEVEL_INFO, strformat(
           F("LA66 : Process Query: %s -> %s"),
           C023_AT_commands::toString(_queryPending).c_str(),
           receivedData.c_str()));

  _queryPending = C023_AT_commands::AT_cmd::Unknown;
  return true;

}

void C023_data_struct::sendQuery(C023_AT_commands::AT_cmd at_cmd, bool prioritize)
{
  if (C023_easySerial) {
    if (prioritize) {
      _queuedQueries.push_front(static_cast<size_t>(at_cmd));
    } else {
      _queuedQueries.push_back(static_cast<size_t>(at_cmd));
    }

    if (loglevelActiveFor(LOG_LEVEL_INFO)) {
      const String query = concat(C023_AT_commands::toString(at_cmd), F("=?"));
      addLog(LOG_LEVEL_INFO, concat(F("LA66 : Add to queue: "), query));
    }
  }
}

void C023_data_struct::sendNextQueuedQuery()
{
  if (!queryPending() && !_queuedQueries.empty()) {
    _queryPending = static_cast<C023_AT_commands::AT_cmd>(_queuedQueries.front());
    _queuedQueries.pop_front();
    const String query = concat(C023_AT_commands::toString(_queryPending), F("=?"));
    addLog(LOG_LEVEL_INFO, concat(F("LA66 : Queried "), query));

    C023_easySerial->println(query);
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
    it->second.value = std::move(value);
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

String C023_data_struct::getValueFromReceivedBinaryData(int& port, const String& receivedData)
{
  port = -1;
  int pos = receivedData.indexOf(':');

  if (pos == -1) { return EMPTY_STRING; }
  port = receivedData.substring(0, pos).toInt();

  addLog(LOG_LEVEL_INFO, concat(
           F("LA66 fromHex: "), receivedData.substring(pos + 1)));

  return stringFromHexArray(receivedData.substring(pos + 1));
}

#endif // ifdef USES_C023
