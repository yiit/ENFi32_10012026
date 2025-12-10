#ifndef CONTROLLER_STRUCT_C023_DATA_STRUCT_H
#define CONTROLLER_STRUCT_C023_DATA_STRUCT_H

#include "../Helpers/_CPlugin_Helper.h"

#ifdef USES_C023

# include "../Controller_config/C023_AT_commands.h"
# include "../Controller_config/C023_config.h"


class ESPeasySerial;


struct C023_data_struct {
public:

  C023_data_struct();

  ~C023_data_struct();

  void reset();

  bool init(
    const C023_ConfigStruct& config,
    taskIndex_t              sampleSet_Initiator);

  bool isInitialized() const {
    return C023_easySerial != nullptr;
  }

  bool hasJoined();

  bool useOTAA();

  bool command_finished() const;

  bool txUncnfBytes(const uint8_t *data,
                    uint8_t        size,
                    uint8_t        port);

  bool txHexBytes(const String& data,
                  uint8_t       port);

  bool txUncnf(const String& data,
               uint8_t       port);

  bool setSF(uint8_t sf);

  bool setAdaptiveDataRate(bool enabled);

  bool initOTAA(const String& AppEUI,
                const String& AppKey,
                const String& DevEUI);

  bool initABP(const String& addr,
               const String& AppSKey,
               const String& NwkSKey);

  String   sendRawCommand(const String& command);

  int      getVbat();

  String   peekLastError();

  String   getLastError();

  String   getDataRate();

  int      getRSSI();

  uint32_t getRawStatus();


  bool     getFrameCounters(uint32_t& dnctr,
                            uint32_t& upctr);

  bool     setFrameCounters(uint32_t dnctr,
                            uint32_t upctr);

  // Cached data, only changing occasionally.

  String  getDevaddr();

  String  hweui();

  String  sysver();

  uint8_t getSampleSetCount() const;

  uint8_t getSampleSetCount(taskIndex_t taskIndex);

  float   getLoRaAirTime(uint8_t pl) const;

  void    async_loop();

  bool    writeCachedValues(KeyValueWriter*writer);

private:

  String get(C023_AT_commands::AT_cmd at_cmd);
  int    getInt(C023_AT_commands::AT_cmd at_cmd,
                int                      errorvalue);
  bool   processReceived(const String& receivedData);

  bool   processPendingQuery(const String& receivedData);

  void   sendQuery(C023_AT_commands::AT_cmd at_cmd,
                   bool                     prioritize = false);

  void   sendNextQueuedQuery();

  bool   queryPending() const {
    return _queryPending != C023_AT_commands::AT_cmd::Unknown;
  }

  void          cacheValue(C023_AT_commands::AT_cmd at_cmd,
                           const String           & value);
  void          cacheValue(C023_AT_commands::AT_cmd at_cmd,
                           String                && value);

  static String getValueFromReceivedData(const String& receivedData);

  // Decode HEX stream
  static String getValueFromReceivedBinaryData(int         & port,
                                               const String& receivedData);

  std::map<size_t, C023_timestamped_value>_cachedValues;
  std::list<size_t>                       _queuedQueries;
  C023_AT_commands::AT_cmd                _queryPending = C023_AT_commands::AT_cmd::Unknown;


  ESPeasySerial *C023_easySerial    = nullptr;
  unsigned long  _baudrate          = 9600;
  uint8_t        sampleSetCounter   = 0;
  taskIndex_t    sampleSetInitiator = INVALID_TASK_INDEX;
  int8_t         _resetPin          = -1;

  C023_ConfigStruct::EventFormatStructure_e _eventFormatStructure = C023_ConfigStruct::EventFormatStructure_e::PortNr_in_eventPar;


  String _fromLA66;

};

#endif // ifdef USES_C023

#endif // ifndef CONTROLLER_STRUCT_C023_DATA_STRUCT_H
