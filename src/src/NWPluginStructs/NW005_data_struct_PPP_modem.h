#ifndef NWPLUGINSTRUCT_NW005_DATA_STRUCT_PPP_MODEM_H
#define NWPLUGINSTRUCT_NW005_DATA_STRUCT_PPP_MODEM_H


#include "../../_NWPlugin_Helper.h"
#ifdef USES_NW005

# include "../Helpers/StringGenerator_GPIO.h"

# include <PPP.h>


namespace ESPEasy {
namespace net {
namespace ppp {

struct NW005_modem_task_data {
  ppp_modem_model_t model     = PPP_MODEM_GENERIC;
  uint8_t           uart_num  = 1;
  int               baud_rate = 115200;
  bool              initializing{};
  bool              modem_initialized{};
  String            logString;

  // This is C-code, so not set to nullptr, but to NULL
  TaskHandle_t modem_taskHandle = NULL;

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

  void   webform_load(EventStruct *event);
  void   webform_save(EventStruct *event);

  bool   webform_getPort(String& str);

  bool   init(EventStruct *event);

  bool   exit(EventStruct *event);


  void   testWrite();

  void   testRead();

private:

  String NW005_formatGpioLabel(uint32_t          key,
                               PinSelectPurpose& purpose,
                               bool              shortNotation = false) const;

  NW005_modem_task_data _modem_task_data;


};

} // namespace ppp
} // namespace net
} // namespace ESPEasy

#endif // ifdef USES_NW005

#endif // ifndef NWPLUGINSTRUCT_NW005_DATA_STRUCT_PPP_MODEM_H
