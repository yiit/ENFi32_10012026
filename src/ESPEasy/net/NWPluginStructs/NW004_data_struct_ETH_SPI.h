#pragma once

#include "../_NWPlugin_Helper.h"
#ifdef USES_NW004


namespace ESPEasy {
namespace net {
namespace eth {

struct NW004_data_struct_ETH_SPI : public NWPluginData_base {

  NW004_data_struct_ETH_SPI(networkIndex_t networkIndex);
  ~NW004_data_struct_ETH_SPI();


  void                         webform_load(EventStruct *event);
  void                         webform_save(EventStruct *event);

  bool                         webform_getPort(String& str);

  bool                         init(EventStruct *event);

  bool                         exit(EventStruct *event);

  NWPluginData_static_runtime& getNWPluginData_static_runtime();

private:

  static void onEvent(arduino_event_id_t   event,
                      arduino_event_info_t info);

  network_event_handle_t nw_event_id = 0;

};


} // namespace eth
} // namespace net
} // namespace ESPEasy


#endif // ifdef USES_NW004
