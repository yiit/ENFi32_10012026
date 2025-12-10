#ifndef CONTROLLER_CONFIG_C023_CONFIG_H
#define CONTROLLER_CONFIG_C023_CONFIG_H

#include "../Helpers/_CPlugin_Helper.h"

#ifdef USES_C023

// Forward declaration
struct C023_data_struct;


# define C023_DEVICE_EUI_LEN          17
# define C023_DEVICE_ADDR_LEN         33
# define C023_NETWORK_SESSION_KEY_LEN 33
# define C023_APP_SESSION_KEY_LEN     33
# define C023_USE_OTAA                0
# define C023_USE_ABP                 1

struct C023_ConfigStruct
{
  enum class LoRaWANclass_e {
    A,
    B,
    C

  };

  enum class EventFormatStructure_e {
    PortNr_in_eventPar,
    PortNr_as_first_eventvalue,
    PortNr_both_eventPar_eventvalue

  };

  C023_ConfigStruct() = default;

  void                   validate();

  void                   reset();

  // Send all to the web interface
  void                   webform_load(C023_data_struct*C023_data);

  // Collect all data from the web interface
  void                   webform_save();

  LoRaWANclass_e         getClass() const       { return static_cast<LoRaWANclass_e>(LoRaWAN_Class); }

  EventFormatStructure_e getEventFormat() const { return static_cast<EventFormatStructure_e>(eventFormat); }

  char          DeviceEUI[C023_DEVICE_EUI_LEN]                  = { 0 };
  char          DeviceAddr[C023_DEVICE_ADDR_LEN]                = { 0 };
  char          NetworkSessionKey[C023_NETWORK_SESSION_KEY_LEN] = { 0 };
  char          AppSessionKey[C023_APP_SESSION_KEY_LEN]         = { 0 };
  unsigned long baudrate                                        = 9600;
  int8_t        rxpin                                           = -1;
  int8_t        txpin                                           = -1;
  int8_t        resetpin                                        = -1;
  uint8_t       sf                                              = 7;
  uint8_t       eventFormat                                     = static_cast<uint8_t>(EventFormatStructure_e::PortNr_in_eventPar);
  uint8_t       joinmethod                                      = C023_USE_OTAA;
  uint8_t       serialPort                                      = 0;
  uint8_t       LoRaWAN_Class                                   = static_cast<uint8_t>(LoRaWANclass_e::A);
  uint8_t       adr                                             = 0;
  uint32_t      rx2_freq                                        = 0;

};

DEF_UP(C023_ConfigStruct);

#endif // ifdef USES_C023

#endif // ifndef CONTROLLER_CONFIG_C023_CONFIG_H
