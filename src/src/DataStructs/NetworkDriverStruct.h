#ifndef DATASTRUCTS_NETWORKDRIVERSTRUCT_H
#define DATASTRUCTS_NETWORKDRIVERSTRUCT_H

#include "../../ESPEasy_common.h"

#include "../DataTypes/NetworkIndex.h"

#include <vector>

/*********************************************************************************************\
* NetworkDriverStruct
\*********************************************************************************************/
struct NetworkDriverStruct
{
  NetworkDriverStruct() = default;

  bool           onlySingleInstance = true;
  bool           alwaysPresent      = false;
  networkIndex_t fixedNetworkIndex  = INVALID_NETWORK_INDEX;


};

#endif // ifndef DATASTRUCTS_NETWORKDRIVERSTRUCT_H
