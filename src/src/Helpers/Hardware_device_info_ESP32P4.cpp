#include "../Helpers/Hardware_device_info.h"

#ifdef ESP32P4

#include <soc/efuse_reg.h>

const __FlashStringHelper* getChipModel(uint32_t chip_model, uint32_t chip_revision, uint32_t pkg_version, bool single_core)
{
  if (18 == chip_model) { // ESP32-P4
    return F("ESP32-P4");
  }

  return F("Unknown");
}

int32_t getEmbeddedFlashSize()
{
  // TODO TD-er: Implement
  return 16;
}

int32_t getEmbeddedPSRAMSize()
{
  const uint32_t psram_cap = REG_GET_FIELD(EFUSE_RD_MAC_SYS_2_REG, EFUSE_PSRAM_CAP);

 // TODO TD-er: Must check whether this is true
  switch (psram_cap) {
    case 0: return 0;
    case 1: return 8;
    case 2: return 2;
  }

  // Unknown value, thus mark as negative value
  return -1 * static_cast<int32_t>(psram_cap);
}

# ifndef isPSRAMInterfacePin
bool isPSRAMInterfacePin(int gpio) {
  // TODO TD-er: Implement
  return false;
}

# endif // ifndef isPSRAMInterfacePin

bool isFlashInterfacePin_ESPEasy(int gpio) {
  // TODO TD-er: Implement
  return false;
}

#endif // ifdef ESP32P4
