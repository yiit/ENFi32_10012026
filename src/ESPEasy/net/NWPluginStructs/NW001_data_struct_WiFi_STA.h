#pragma once

#include "../_NWPlugin_Helper.h"
#ifdef USES_NW001


namespace ESPEasy {
namespace net {
namespace wifi {

struct NW001_data_struct_WiFi_STA : public NWPluginData_base {

  NW001_data_struct_WiFi_STA(networkIndex_t networkIndex);
  ~NW001_data_struct_WiFi_STA();


  void webform_load(EventStruct *event);
  void webform_save(EventStruct *event);

  bool webform_getPort(String& str);

  bool init(EventStruct *event);

  bool exit(EventStruct *event);

private:

  static void onEvent(arduino_event_id_t   event,
                      arduino_event_info_t info);

  network_event_handle_t nw_event_id = 0;

};


} // namespace wifi
} // namespace net
} // namespace ESPEasy


#endif // ifdef USES_NW001
