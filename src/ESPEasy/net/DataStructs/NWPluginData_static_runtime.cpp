#include "../DataStructs/NWPluginData_static_runtime.h"


namespace ESPEasy {
namespace net {

void NWPluginData_static_runtime::clear()
{
  _connectedStats.clear();
  _gotIPStats.clear();
#if FEATURE_USE_IPV6
  _gotIP6Stats.clear();
#endif

  // FIXME TD-er: Should also clear dns cache?
}

} // namespace net
} // namespace ESPEasy
