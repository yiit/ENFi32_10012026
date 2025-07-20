#ifndef NWPLUGINSTRUCT_NW005_DATA_STRUCT_PPP_MODEM_H
#define NWPLUGINSTRUCT_NW005_DATA_STRUCT_PPP_MODEM_H


#include "../../_NWPlugin_Helper.h"
#ifdef USES_NW005

# include "../Helpers/StringGenerator_GPIO.h"

# include <PPP.h>

struct NW005_modem_task_data {
  ppp_modem_model_t model     = PPP_MODEM_GENERIC;
  uint8_t           uart_num  = 1;
  int               baud_rate = 115200;
  bool              initializing{};
  bool              modem_initialized{};
  String            logString;
  TaskHandle_t      modem_taskHandle = nullptr;

};


struct NW005_data_struct_PPP_modem : public NWPluginData_base {

  NW005_data_struct_PPP_modem(networkIndex_t networkIndex);
  ~NW005_data_struct_PPP_modem();

  String getRSSI() const;
  String getBER() const;
  bool   attached() const;
  String IMEI() const;
  String operatorName() const;

  void   webform_load_UE_system_information();

  void   webform_load(struct EventStruct *event);
  void   webform_save(struct EventStruct *event);

  bool   webform_getPort(String& str);

  bool   init(struct EventStruct *event);

  bool   exit(struct EventStruct *event);


  void   testWrite();

  void   testRead();

private:

  String NW005_formatGpioLabel(uint32_t          key,
                         PinSelectPurpose& purpose,
                         bool              shortNotation = false) const;

  NW005_modem_task_data _modem_task_data;


};


#endif // ifdef USES_NW005

#endif // ifndef NWPLUGINSTRUCT_NW005_DATA_STRUCT_PPP_MODEM_H
