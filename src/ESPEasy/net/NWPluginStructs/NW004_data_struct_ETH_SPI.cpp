#include "../NWPluginStructs/NW004_data_struct_ETH_SPI.h"

#ifdef USES_NW004

# include "../../../src/Globals/Settings.h"

# include "../../../src/Helpers/ESPEasy_time_calc.h"
# include "../../../src/Helpers/LongTermOnOffTimer.h"
# include "../../../src/Helpers/StringConverter.h"

# include "../Globals/NetworkState.h"

# include "../eth/ESPEasyEth.h"

# define NW_PLUGIN_ID  4
# define NW_PLUGIN_INTERFACE   ETH

namespace ESPEasy {
namespace net {
namespace eth {

static NWPluginData_static_runtime stats_and_cache(&NW_PLUGIN_INTERFACE);

NW004_data_struct_ETH_SPI::NW004_data_struct_ETH_SPI(networkIndex_t networkIndex)
  : NWPluginData_base(nwpluginID_t(NW_PLUGIN_ID), networkIndex, &NW_PLUGIN_INTERFACE)
{
  stats_and_cache.clear(networkIndex);
  nw_event_id = Network.onEvent(NW004_data_struct_ETH_SPI::onEvent);
}

NW004_data_struct_ETH_SPI::~NW004_data_struct_ETH_SPI()
{
  if (nw_event_id != 0) {
    Network.removeEvent(nw_event_id);
  }
  nw_event_id = 0;
  stats_and_cache.processEvent_and_clear();
}

void NW004_data_struct_ETH_SPI::webform_load(EventStruct *event) {}

void NW004_data_struct_ETH_SPI::webform_save(EventStruct *event) {}

bool NW004_data_struct_ETH_SPI::webform_getPort(String& str)     { return true; }

bool NW004_data_struct_ETH_SPI::init(EventStruct *event)
{
  auto data = getNWPluginData_static_runtime();

  if (data) { 
    ESPEasy::net::eth::ETHConnectRelaxed(*data);
  }

  return true;
}

bool NW004_data_struct_ETH_SPI::exit(EventStruct *event)
{
  ETH.end();
  stats_and_cache.processEvents();

  return true;
}

NWPluginData_static_runtime * NW004_data_struct_ETH_SPI::getNWPluginData_static_runtime() { return &stats_and_cache; }

void                          NW004_data_struct_ETH_SPI::onEvent(arduino_event_id_t   event,
                                                                 arduino_event_info_t info)
{
  switch (event)
  {
    case ARDUINO_EVENT_ETH_START:
      stats_and_cache.mark_start();
      break;
    case ARDUINO_EVENT_ETH_STOP:
      stats_and_cache.mark_stop();
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      stats_and_cache.mark_connected();
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      stats_and_cache.mark_disconnected();
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      stats_and_cache.mark_got_IP();
      break;
# if FEATURE_USE_IPV6
    case ARDUINO_EVENT_ETH_GOT_IP6:
      stats_and_cache.mark_got_IPv6(&info.got_ip6);
      break;
# endif // if FEATURE_USE_IPV6
    case ARDUINO_EVENT_ETH_LOST_IP:
      stats_and_cache.mark_lost_IP();
      break;

    default: break;
  }
}

} // namespace eth
} // namespace net
} // namespace ESPEasy

#endif // ifdef USES_NW004
