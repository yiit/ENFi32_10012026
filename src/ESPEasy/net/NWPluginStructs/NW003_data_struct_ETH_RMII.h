#pragma once

#include "../_NWPlugin_Helper.h"
#ifdef USES_NW003


namespace ESPEasy {
namespace net {
namespace eth {

struct NW003_data_struct_ETH_RMII : public NWPluginData_base {

  NW003_data_struct_ETH_RMII(networkIndex_t networkIndex);
  ~NW003_data_struct_ETH_RMII();


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


#endif // ifdef USES_NW003
