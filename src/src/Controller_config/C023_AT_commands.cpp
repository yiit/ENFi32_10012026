#include "../Controller_config/C023_AT_commands.h"

#include "../Helpers/StringConverter.h"


bool C023_timestamped_value::expired() const
{
    // TODO TD-er: Implement
    return false;    
}

const __FlashStringHelper * C023_AT_commands::toFlashString(C023_AT_commands::AT_cmd at_cmd)
{

  // Uncrustify must not be used on macros, so turn it off
  // *INDENT-OFF*
  #define C023_AT_STR(S) case C023_AT_commands::AT_cmd::S:  return F(#S);
  // Uncrustify must not be used on macros, but we're now done, so turn Uncrustify on again        
 // *INDENT-ON*

  switch (at_cmd)
  {
  C023_AT_STR(UUID);
  C023_AT_STR(VER);
  C023_AT_STR(APPEUI);
  C023_AT_STR(APPKEY);
  C023_AT_STR(APPSKEY);
  C023_AT_STR(DADDR);
  C023_AT_STR(DEUI);
  C023_AT_STR(NWKID);
  C023_AT_STR(NWKSKEY);
  C023_AT_STR(CFM);
  C023_AT_STR(NJM);
  C023_AT_STR(NJS);
  C023_AT_STR(RECV);
  C023_AT_STR(RECVB);
  C023_AT_STR(ADR);
  C023_AT_STR(CLASS);
  C023_AT_STR(DCS);
  C023_AT_STR(DR);
  C023_AT_STR(FCD);
  C023_AT_STR(FCU);
  C023_AT_STR(JN1DL);
  C023_AT_STR(JN2DL);
  C023_AT_STR(PNM);
  C023_AT_STR(RX1DL);
  C023_AT_STR(RX2DL);
  C023_AT_STR(RX2DR);
  C023_AT_STR(RX2FQ);
  C023_AT_STR(TXP);
  C023_AT_STR(RSSI);
  C023_AT_STR(SNR);
  C023_AT_STR(PORT);
  C023_AT_STR(CHS);
  C023_AT_STR(SLEEP);
  C023_AT_STR(BAT);
  C023_AT_STR(RJTDC);
  C023_AT_STR(RPL);
  C023_AT_STR(TIMESTAMP);
  C023_AT_STR(LEAPSEC);
  C023_AT_STR(SYNCMOD);
  C023_AT_STR(SYNCTDC);
  C023_AT_STR(DDETECT);
  C023_AT_STR(SETMAXNBTRANS);
  case C023_AT_commands::AT_cmd::Unknown: break;
  }
  return F("Unknown");
#undef C023_AT_STR
}

String                      C023_AT_commands::toString(C023_AT_commands::AT_cmd at_cmd) { return concat(F("AT+"), toFlashString(at_cmd)); }

const __FlashStringHelper * C023_AT_commands::toDisplayString(C023_AT_commands::AT_cmd at_cmd)
{
  switch (at_cmd)
  {
    case C023_AT_commands::AT_cmd::Unknown: break;

    case C023_AT_commands::AT_cmd::UUID:          return F("UUID");
    case C023_AT_commands::AT_cmd::VER:           return F("Image Version and Frequency Band");          // 2.4 AT+VER
    case C023_AT_commands::AT_cmd::APPEUI:        return F("Application EUI");                           // 3.1 AT+APPEUI
    case C023_AT_commands::AT_cmd::APPKEY:        return F("Application Key");                           // 3.2 AT+APPKEY
    case C023_AT_commands::AT_cmd::APPSKEY:       return F("Application Session Key");                   // 3.3 AT+APPSKEY
    case C023_AT_commands::AT_cmd::DADDR:         return F("Device Address");                            // 3.4 AT+DADDR
    case C023_AT_commands::AT_cmd::DEUI:          return F("Device EUI");                                // 3.5 AT+DEUI
    case C023_AT_commands::AT_cmd::NWKID:         return F("Network ID");                                // 3.6 AT+NWKID
    case C023_AT_commands::AT_cmd::NWKSKEY:       return F("Network Session Key");                       // 3.7 AT+NWKSKEY
    case C023_AT_commands::AT_cmd::CFM:           return F("Confirm Mode");                              // 4.1 AT+CFM
    case C023_AT_commands::AT_cmd::NJM:           return F("LoRa® Network Join Mode");                   // 4.3 AT+NJM
    case C023_AT_commands::AT_cmd::NJS:           return F("LoRa® Network Join Status");                 // 4.4 AT+NJS
    case C023_AT_commands::AT_cmd::RECV:          return F("Print Last Received Data in Raw Format");    // 4.5 AT+RECV
    case C023_AT_commands::AT_cmd::RECVB:         return F("Print Last Received Data in Binary Format"); // 4.6 AT+RECVB
    case C023_AT_commands::AT_cmd::ADR:           return F("Adaptive Rate");                             // 5.1 AT+ADR
    case C023_AT_commands::AT_cmd::CLASS:         return F("LoRa® Class");                               // 5.2 AT+CLASS
    case C023_AT_commands::AT_cmd::DCS:           return F("Duty Cycle Setting");                        // 5.3 AT+DCS
    case C023_AT_commands::AT_cmd::DR:            return F("Data Rate");                                 // 5.4 AT+DR
    case C023_AT_commands::AT_cmd::FCD:           return F("Frame Counter Downlink");                    // 5.5 AT+FCD
    case C023_AT_commands::AT_cmd::FCU:           return F("Frame Counter Uplink");                      // 5.6 AT+FCU
    case C023_AT_commands::AT_cmd::JN1DL:         return F("Join Accept Delay1");                        // 5.7 AT+JN1DL
    case C023_AT_commands::AT_cmd::JN2DL:         return F("Join Accept Delay2");                        // 5.8 AT+JN2DL
    case C023_AT_commands::AT_cmd::PNM:           return F("Public Network Mode");                       // 5.9 AT+PNM
    case C023_AT_commands::AT_cmd::RX1DL:         return F("Receive Delay1");                            // 5.10 AT+RX1DL
    case C023_AT_commands::AT_cmd::RX2DL:         return F("Receive Delay2");                            // 5.11 AT+RX2DL
    case C023_AT_commands::AT_cmd::RX2DR:         return F("Rx2 Window Data Rate");                      // 5.12 AT+RX2DR
    case C023_AT_commands::AT_cmd::RX2FQ:         return F("Rx2 Window Frequency");                      // 5.13 AT+RX2FQ
    case C023_AT_commands::AT_cmd::TXP:           return F("Transmit Power");                            // 5.14 AT+TXP
    case C023_AT_commands::AT_cmd::RSSI:          return F("RSSI of the Last Received Packet");          // 5.15 AT+RSSI
    case C023_AT_commands::AT_cmd::SNR:           return F("SNR of the Last Received Packet");           // 5.16 AT+SNR
    case C023_AT_commands::AT_cmd::PORT:          return F("Application Port");                          // 5.17 AT+PORT
    case C023_AT_commands::AT_cmd::CHS:           return F("Single Channel Mode");                       // 5.18 AT+ CHS
    case C023_AT_commands::AT_cmd::SLEEP:         return F("Sleep mode");                                // 5.20 AT+SLEEP
    case C023_AT_commands::AT_cmd::BAT:           return F("Current battery voltage in Mv");             // 5.22 AT+BAT
    case C023_AT_commands::AT_cmd::RJTDC:         return F("ReJoin data transmission interval in min");  // 5.23 AT+RJTDC
    case C023_AT_commands::AT_cmd::RPL:           return F("Response level");                            // 5.24 AT+RPL
    case C023_AT_commands::AT_cmd::TIMESTAMP:     return F("UNIX timestamp in second");                  // 5.25 AT+TIMESTAMP
    case C023_AT_commands::AT_cmd::LEAPSEC:       return F("Leap Second");                               // 5.26 AT+LEAPSEC
    case C023_AT_commands::AT_cmd::SYNCMOD:       return F("Time synchronization method");               // 5.27 AT+SYNCMOD
    case C023_AT_commands::AT_cmd::SYNCTDC:       return F("Time synchronization interval in day");      // 5.28 AT+SYNCTDC
    case C023_AT_commands::AT_cmd::DDETECT:       return F("Downlink detection");                        // 5.29 AT+DDETECT
    case C023_AT_commands::AT_cmd::SETMAXNBTRANS: return F("Max nbtrans in LinkADR");                    // 5.30 AT+SETMAXNBTRANS
  }
  return F("");
}

C023_AT_commands::AT_cmd C023_AT_commands::determineReceivedDataType(const String& receivedData)
{
  // Uncrustify must not be used on macros, so turn it off
  // *INDENT-OFF*
  #define MATCH_STRING(S) if (searchStr.startsWith(F(#S))) return C023_AT_commands::AT_cmd::S; 
  // Uncrustify must not be used on macros, but we're now done, so turn Uncrustify on again        
 // *INDENT-ON*


  if ((receivedData.length() > 4) && receivedData.startsWith(F("AT+"))) {
    String searchStr = receivedData.substring(3);
    searchStr.trim();

    switch (searchStr[0])
    {
      case 'A':
        MATCH_STRING(ADR);
        MATCH_STRING(APPEUI);
        MATCH_STRING(APPKEY);
        MATCH_STRING(APPSKEY);
        break;
      case 'B':
        MATCH_STRING(BAT);
        break;
      case 'C':
        MATCH_STRING(CFM);
        MATCH_STRING(CHS);
        MATCH_STRING(CLASS);
        break;
      case 'D':
        MATCH_STRING(DADDR);
        MATCH_STRING(DCS);
        MATCH_STRING(DDETECT);
        MATCH_STRING(DEUI);
        MATCH_STRING(DR);
        break;
      case 'F':
        MATCH_STRING(FCD);
        MATCH_STRING(FCU);
        break;
      case 'J':
        MATCH_STRING(JN1DL);
        MATCH_STRING(JN2DL);
        break;
      case 'L':
        MATCH_STRING(LEAPSEC);
        break;
      case 'N':
        MATCH_STRING(NJM);
        MATCH_STRING(NJS);
        MATCH_STRING(NWKID);
        MATCH_STRING(NWKSKEY);
        break;
      case 'P':
        MATCH_STRING(PNM);
        MATCH_STRING(PORT);
        break;
      case 'R':
        MATCH_STRING(RECV);
        MATCH_STRING(RECVB);
        MATCH_STRING(RJTDC);
        MATCH_STRING(RPL);
        MATCH_STRING(RSSI);
        MATCH_STRING(RX1DL);
        MATCH_STRING(RX2DL);
        MATCH_STRING(RX2DR);
        MATCH_STRING(RX2FQ);
        break;
      case 'S':
        MATCH_STRING(SETMAXNBTRANS);
        MATCH_STRING(SLEEP);
        MATCH_STRING(SNR);
        MATCH_STRING(SYNCMOD);
        MATCH_STRING(SYNCTDC);
        break;
      case 'T':
        MATCH_STRING(TIMESTAMP);
        MATCH_STRING(TXP);
        break;
      case 'U':
        MATCH_STRING(UUID);
        break;
      case 'V':
        MATCH_STRING(VER);
        break;
    }
  }
  #undef MATCH_STRING
  return C023_AT_commands::AT_cmd::Unknown;
}

C023_AT_commands::AT_cmd C023_AT_commands::decode(const String& receivedData, String& value)
{
  const C023_AT_commands::AT_cmd res = determineReceivedDataType(receivedData);

  if (res != C023_AT_commands::AT_cmd::Unknown) {
    const int pos = receivedData.indexOf('=');

    if (pos == -1) {
      value.clear();
    } else {
      value = receivedData.substring(pos + 1);
    }
  }
  return res;
}

KeyValueStruct C023_AT_commands::getKeyValue(C023_AT_commands::AT_cmd at_cmd, const String& value, bool extendedValue)
{
  if (at_cmd != C023_AT_commands::AT_cmd::Unknown && !value.isEmpty()) {
    if ((at_cmd == C023_AT_commands::AT_cmd::NJS) ||
        (at_cmd == C023_AT_commands::AT_cmd::ADR) ||
        (at_cmd == C023_AT_commands::AT_cmd::DCS) ||
        (at_cmd == C023_AT_commands::AT_cmd::PNM) ||
        (at_cmd == C023_AT_commands::AT_cmd::CHS) ||        
        (at_cmd == C023_AT_commands::AT_cmd::SLEEP) ||
        (at_cmd == C023_AT_commands::AT_cmd::SYNCMOD))
    {
      const bool boolValue = value.toInt() != 0;

      return KeyValueStruct(
        extendedValue
            ? toDisplayString(at_cmd)
            : toFlashString(at_cmd),
        boolValue);
    }

    return KeyValueStruct(
      extendedValue
            ? toDisplayString(at_cmd)
            : toFlashString(at_cmd),
      value,
      KeyValueStruct::Format::PreFormatted);
  }
  return KeyValueStruct();
}

