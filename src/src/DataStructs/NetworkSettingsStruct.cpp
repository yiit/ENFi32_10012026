#include "../DataStructs/NetworkSettingsStruct.h"

#include "../../ESPEasy_common.h"

#include "../CustomBuild/ESPEasyLimits.h"
#include "../ESPEasyCore/ESPEasy_Log.h"
#include "../ESPEasyCore/ESPEasyNetwork.h"
#include "../Helpers/Misc.h"
#include "../Helpers/Networking.h"
#include "../Helpers/StringConverter.h"


#include <IPAddress.h>
#include <WString.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>


NetworkSettingsStruct::NetworkSettingsStruct()
{
  memset(this, 0, sizeof(NetworkSettingsStruct));
}

void NetworkSettingsStruct::reset() {
  // Need to make sure every byte between the members is also zero
  // Otherwise the checksum will fail and settings will be saved too often.
  memset(this, 0, sizeof(NetworkSettingsStruct));


}


void NetworkSettingsStruct::validate() {


}

