#ifndef DATASTRUCTS_NETWORKSETTINGSSTRUCT_H
#define DATASTRUCTS_NETWORKSETTINGSSTRUCT_H

/*********************************************************************************************\
* NetworkSettingsStruct definition
\*********************************************************************************************/
#include "../../ESPEasy_common.h"

#include <memory> // For std::shared_ptr
#include <new>    // for std::nothrow

#include "../Globals/Plugins.h"
#include "../Helpers/Memory.h"

namespace ESPEasy {
namespace net {


struct NetworkSettingsStruct
{
  // ********************************************************************************
  //   IDs of network settings, used to generate web forms
  // ********************************************************************************
  enum VarType {
    NETWORK_IP,
    NETWORK_PORT,
    NETWORK_USER,
    NETWORK_PASS,
    NETWORK_TIMEOUT,


    // Keep this as last, is used to loop over all parameters
    NETWORK_ENABLED

  };


  NetworkSettingsStruct();

  void reset();

  void validate();


  uint8_t      IP[4];
  unsigned int Port;
  unsigned int ClientTimeout;

private:

};


typedef std::shared_ptr<NetworkSettingsStruct> NetworkSettingsStruct_ptr_type;

/*
 # ifdef USE_SECOND_HEAP
 #define MakeNetworkSettings(T) HeapSelectIram ephemeral; NetworkSettingsStruct_ptr_type T(new (std::nothrow)  NetworkSettingsStruct());
 #else
 */
#define MakeNetworkSettings(T) void *calloc_ptr = special_calloc(1, sizeof(NetworkSettingsStruct)); \
        NetworkSettingsStruct_ptr_type T(new (calloc_ptr)  NetworkSettingsStruct());

// #endif

// Check to see if MakeNetworkSettings was successful
#define AllocatedNetworkSettings() (NetworkSettings.get() != nullptr)

}
}


#endif // DATASTRUCTS_NETWORKSETTINGSSTRUCT_H
