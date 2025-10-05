#include "../WebServer/SysInfoPage.h"

#if defined(WEBSERVER_SYSINFO) || SHOW_SYSINFO_JSON

# include "../../ESPEasy-Globals.h"
# include "../../ESPEasy/net/ESPEasyNetwork.h"
# include "../../ESPEasy/net/Globals/ESPEasyWiFi.h"
# include "../../ESPEasy/net/Globals/ESPEasyWiFiEvent.h"
# include "../../ESPEasy/net/Globals/NetworkState.h"
# include "../../ESPEasy/net/Helpers/NWAccessControl.h"
# include "../../ESPEasy/net/eth/ESPEasyEth.h"
# include "../../ESPEasy/net/wifi/ESPEasyWifi.h"
# include "../Commands/Diagnostic.h"
# include "../CustomBuild/CompiletimeDefines.h"
# include "../DataStructs/RTCStruct.h"
# include "../Globals/CRCValues.h"
# include "../Globals/ESPEasy_time.h"
# include "../Globals/RTC.h"
# include "../Globals/Settings.h"
# include "../Helpers/Convert.h"
# include "../Helpers/ESPEasyStatistics.h"
# include "../Helpers/ESPEasy_Storage.h"
# include "../Helpers/Hardware_device_info.h"
# include "../Helpers/KeyValueWriter_JSON.h"
# include "../Helpers/Memory.h"
# include "../Helpers/Misc.h"
# include "../Helpers/Networking.h"
# include "../Helpers/OTA.h"
# include "../Helpers/StringConverter.h"
# include "../Helpers/StringGenerator_GPIO.h"
# include "../Helpers/StringGenerator_System.h"
# include "../Helpers/StringProvider.h"
# include "../Static/WebStaticData.h"
# include "../WebServer/AccessControl.h"
# include "../WebServer/ESPEasy_WebServer.h"
# include "../WebServer/HTML_wrappers.h"
# include "../WebServer/Markup.h"
# include "../WebServer/Markup_Buttons.h"

# if FEATURE_MQTT
#  include "../Globals/MQTT.h"
#  include "../ESPEasyCore/Controller.h" // For finding enabled MQTT controller
# endif // if FEATURE_MQTT


# if FEATURE_INTERNAL_TEMPERATURE
#  include "../Helpers/Hardware_temperature_sensor.h"
# endif

# ifdef ESP32
#  include <esp_partition.h>
# endif // ifdef ESP32


# if SHOW_SYSINFO_JSON

// ********************************************************************************
// Web Interface sysinfo page
// ********************************************************************************
void handle_sysinfo_json() {
#  ifndef BUILD_NO_RAM_TRACKER
  checkRAM(F("handle_sysinfo"));
#  endif // ifndef BUILD_NO_RAM_TRACKER

  if (!isLoggedIn()) { return; }
  TXBuffer.startJsonStream();
  {
    KeyValueWriter_JSON mainLevelWriter(true);
    {

      auto writer = mainLevelWriter.createChild(F("general"));

      if (writer) {

        writer->write({ F("unit"),       Settings.Unit });
        writer->write({ F("time"),       node_time.getDateTimeString('-', ':', ' ') });
        writer->write({ F("uptime"),     getExtendedValue(LabelType::UPTIME) });
        writer->write({ F("cpu_load"),   getCPUload(), 2 });
#  if FEATURE_INTERNAL_TEMPERATURE
        writer->write({ F("cpu_temp"),   getInternalTemperature(), 2 });
#  endif
        writer->write({ F("loop_count"), getLoopCountPerSec() });
      }
    }
    int freeMem = ESP.getFreeHeap();
    {
      auto writer = mainLevelWriter.createChild(F("mem"));

      if (writer) {
        writer->write({ F("free"),   freeMem });
        writer->write({ F("low_ram"),
#  ifndef BUILD_NO_RAM_TRACKER
                        lowestRAM
#  else // ifndef BUILD_NO_RAM_TRACKER
                        0
#  endif // ifndef BUILD_NO_RAM_TRACKER
                      });
        writer->write({ F("low_ram_fn"),
#  ifndef BUILD_NO_RAM_TRACKER
                        lowestRAMfunction
#  else // ifndef BUILD_NO_RAM_TRACKER
                        0
#  endif // ifndef BUILD_NO_RAM_TRACKER
                      });
        writer->write({ F("stack"),    getCurrentFreeStack() });
        writer->write({ F("low_stack"),
#  ifndef BUILD_NO_RAM_TRACKER
                        lowestFreeStack
#  else // ifndef BUILD_NO_RAM_TRACKER
                        0
#  endif // ifndef BUILD_NO_RAM_TRACKER
                      });
        writer->write({ F("low_stack_fn"),
#  ifndef BUILD_NO_RAM_TRACKER
                        lowestFreeStackfunction
#  else // ifndef BUILD_NO_RAM_TRACKER
                        0
#  endif // ifndef BUILD_NO_RAM_TRACKER
                      });
      }
    }
    {
      auto writer = mainLevelWriter.createChild(F("boot"));

      if (writer) {
        writer->write({ F("last_cause"), getLastBootCauseString() });
        writer->write({ F("counter"), RTC.bootCounter });
        writer->write({ F("reset_reason"), getResetReasonString() });
      }
    }
    {
      auto writer = mainLevelWriter.createChild(F("wifi"));

      if (writer) {
        writer->write({ F("type"), toString(ESPEasy::net::wifi::getConnectionProtocol()) });
        writer->write({ F("rssi"), WiFi.RSSI() });
        writer->write({ F("dhcp"), useStaticIP() ? LabelType::IP_CONFIG_STATIC : LabelType::IP_CONFIG_DYNAMIC });
        writer->write({ F("ip"),   LabelType::IP_ADDRESS });
#  if FEATURE_USE_IPV6

        if (Settings.EnableIPv6()) {
          writer->write({ F("ip6_local"),  LabelType::IP6_LOCAL });
          writer->write({ F("ip6_global"), LabelType::IP6_GLOBAL });
        }
#  endif // if FEATURE_USE_IPV6

        writer->write({ F("subnet"),        LabelType::IP_SUBNET });
        writer->write({ F("gw"),            LabelType::GATEWAY });
        writer->write({ F("dns1"),          LabelType::DNS_1 });
        writer->write({ F("dns2"),          LabelType::DNS_2 });
        writer->write({ F("allowed_range"), ESPEasy::net::describeAllowedIPrange() });
        writer->write({ F("sta_mac"),       LabelType::STA_MAC });
        writer->write({ F("ap_mac"),        LabelType::AP_MAC });
        writer->write({ F("ssid"),          LabelType::SSID });
        writer->write({ F("bssid"),         LabelType::BSSID });
        writer->write({ F("channel"),       LabelType::CHANNEL });
        writer->write({ F("encryption"),    LabelType::ENCRYPTION_TYPE_STA });
        writer->write({ F("connected"),     LabelType::CONNECTED });
        writer->write({ F("ldr"),           LabelType::LAST_DISC_REASON_STR });
        writer->write({ F("reconnects"),    LabelType::NUMBER_RECONNECTS });
        writer->write({ F("ssid1"),         LabelType::WIFI_STORED_SSID1 });
        writer->write({ F("ssid2"),         LabelType::WIFI_STORED_SSID2 });
      }
    }
    {

#  if FEATURE_ETHERNET
      auto writer = mainLevelWriter.createChild(F("ethernet"));

      if (writer) {
        writer->write({ F("ethwifimode"),   LabelType::ETH_WIFI_MODE });
        writer->write({ F("ethconnected"),  LabelType::ETH_CONNECTED });
        writer->write({ F("ethchip"),       LabelType::ETH_CHIP });
        writer->write({ F("ethduplex"),     LabelType::ETH_DUPLEX });
        writer->write({ F("ethspeed"),      LabelType::ETH_SPEED });
        writer->write({ F("ethstate"),      LabelType::ETH_STATE });
        writer->write({ F("ethspeedstate"), LabelType::ETH_SPEED_STATE });
#   if FEATURE_USE_IPV6

        if (Settings.EnableIPv6()) {
          writer->write({ F("ethipv6local"), LabelType::ETH_IP6_LOCAL });
        }
#   endif // if FEATURE_USE_IPV6
      }
#  endif // if FEATURE_ETHERNET
    }
    {
      auto writer = mainLevelWriter.createChild(F("firmware"));

      if (writer) {
        writer->write({ F("build"),         getSystemBuildString() });
        writer->write({ F("notes"),         F(BUILD_NOTES) });
        writer->write({ F("libraries"),     getSystemLibraryString() });
        writer->write({ F("git_version"),   LabelType::GIT_BUILD });
        writer->write({ F("plugins"),       getPluginDescriptionString() });
        writer->write({ F("md5"),           formatToHex_array((const uint8_t *)CRCValues.compileTimeMD5, sizeof(CRCValues.compileTimeMD5)) });
        writer->write({ F("md5_check"),     CRCValues.checkPassed() });
        writer->write({ F("build_time"),    get_build_time() });
        writer->write({ F("filename"),      LabelType::BINARY_FILENAME });
        writer->write({ F("build_platform"), LabelType::BUILD_PLATFORM });
        writer->write({ F("git_head"),      LabelType::GIT_HEAD });
#  ifdef CONFIGURATION_CODE
        writer->write({ F("configuration_code"), LabelType::CONFIGURATION_CODE_LBL });
#  endif // ifdef CONFIGURATION_CODE
      }

    }
    {
      auto writer = mainLevelWriter.createChild(F("esp"));

      if (writer) {
        writer->write({ F("chip_id"),   LabelType::ESP_CHIP_ID });
        writer->write({ F("cpu"),       LabelType::ESP_CHIP_FREQ });
#  ifdef ESP32
        writer->write({ F("xtal_freq"), LabelType::ESP_CHIP_XTAL_FREQ });
        writer->write({ F("abp_freq"),  LabelType::ESP_CHIP_APB_FREQ });
#  endif // ifdef ESP32
        writer->write({ F("board"),     LabelType::BOARD_NAME });
      }
    }
    {
      auto writer = mainLevelWriter.createChild(F("storage"));

      if (writer) {
        // Set to HEX may be something like 0x1640E0.
        // Where manufacturer is 0xE0 and device is 0x4016.
        writer->write({ F("chip_id"), LabelType::FLASH_CHIP_ID });

        if (flashChipVendorPuya()) {
          if (puyaSupport()) {
            writer->write({ F("vendor"), F("puya, supported") });
          } else {
            writer->write({ F("vendor"), F("puya, error") });
          }
        } else {
          writer->write({ F("vendor"), LabelType::FLASH_CHIP_VENDOR });
        }
        writer->write({ F("device"),      LabelType::FLASH_CHIP_MODEL });
        writer->write({ F("real_size"),  getFlashRealSizeInBytes() / 1024 });
        writer->write({ F("ide_size"),   ESP.getFlashChipSize() / 1024 });

        // Please check what is supported for the ESP32
        writer->write({ F("flash_speed"), LabelType::FLASH_CHIP_SPEED });

        writer->write({ F("mode"), getFlashChipMode() });

        writer->write({ F("writes"),        RTC.flashDayCounter });
        writer->write({ F("flash_counter"), RTC.flashCounter });
        writer->write({ F("sketch_size"),   getSketchSize() / 1024 });
        writer->write({ F("sketch_free"),   getFreeSketchSpace() / 1024 });

        writer->write({ F("spiffs_size"),   SpiffsTotalBytes() / 1024 });
        writer->write({ F("spiffs_free"),   SpiffsFreeSpace() / 1024 });
      }
    }

  }
  TXBuffer.endStream();
}

# endif // SHOW_SYSINFO_JSON

# ifdef WEBSERVER_SYSINFO

void handle_sysinfo() {
  #  ifndef BUILD_NO_RAM_TRACKER
  checkRAM(F("handle_sysinfo"));
  #  endif // ifndef BUILD_NO_RAM_TRACKER

  if (!isLoggedIn()) { return; }
  navMenuIndex = MENU_INDEX_TOOLS;
  html_reset_copyTextCounter();
  TXBuffer.startStream();
  sendHeadandTail_stdtemplate(_HEAD);

  addHtml(printWebString);
  addHtml(F("<form>"));

  // the table header
  html_table_class_normal();


  #  ifdef WEBSERVER_GITHUB_COPY

  // Not using addFormHeader() to get the copy button on the same header line as 2nd column
  html_TR();
  html_table_header(F("System Info"), 225);
  addHtml(F("<TH>")); // Needed to get the copy button on the same header line.
  addCopyButton(F("copyText"), F("\\n"), F("Copy info to clipboard"));

  TXBuffer.addFlashString((PGM_P)FPSTR(githublogo));
  serve_JS(JSfiles_e::GitHubClipboard);

  #  else // ifdef WEBSERVER_GITHUB_COPY
  addFormHeader(F("System Info"));

  #  endif // ifdef WEBSERVER_GITHUB_COPY

  handle_sysinfo_basicInfo();

#  ifndef WEBSERVER_SYSINFO_MINIMAL
  handle_sysinfo_memory();
#  endif

  handle_sysinfo_Network();

#  if FEATURE_ETHERNET
  handle_sysinfo_Ethernet();
#  endif // if FEATURE_ETHERNET

#  ifndef WEBSERVER_SYSINFO_MINIMAL
  handle_sysinfo_WiFiSettings();
#  endif

  handle_sysinfo_Firmware();

#  ifndef WEBSERVER_SYSINFO_MINIMAL
  handle_sysinfo_SystemStatus();

  handle_sysinfo_NetworkServices();

  handle_sysinfo_ESP_Board();

  handle_sysinfo_Storage();
#  endif // ifndef WEBSERVER_SYSINFO_MINIMAL


  html_end_table();
  html_end_form();
  sendHeadandTail_stdtemplate(_TAIL);
  TXBuffer.endStream();
}

void handle_sysinfo_basicInfo() {
  addRowLabelValue(LabelType::UNIT_NR);

  if (node_time.systemTimePresent())
  {
    addRowLabelValue(LabelType::LOCAL_TIME);
    #  if FEATURE_EXT_RTC

    if (Settings.ExtTimeSource() != ExtTimeSource_e::None) {
      addRowLabelValue(LabelType::EXT_RTC_UTC_TIME);
    }
    #  endif // if FEATURE_EXT_RTC
    addRowLabelValue(LabelType::TIME_SOURCE);
    addRowLabelValue(LabelType::TIME_WANDER);
  }

  addRowLabel(LabelType::UPTIME);
  {
    addHtml(getExtendedValue(LabelType::UPTIME));
  }

  addRowLabel(LabelType::LOAD_PCT);

  if (wdcounter > 0)
  {
    addHtml(strformat(
              F("%.2f [%%] (LC=%d)"),
              getCPUload(),
              getLoopCountPerSec()));
  }

  static const LabelType::Enum labels[] PROGMEM =
  {
#  if FEATURE_INTERNAL_TEMPERATURE
    LabelType::INTERNAL_TEMPERATURE,
#  endif

    LabelType::CPU_ECO_MODE,

    LabelType::RESET_REASON,
    LabelType::LAST_TASK_BEFORE_REBOOT,
    LabelType::SW_WD_COUNT,
    LabelType::MAX_LABEL
  };
  addRowLabelValues(labels);

  addRowLabel(F("Boot"));
  {
    addHtml(getLastBootCauseString());
    addHtml(strformat(
              F(" (%d)"), static_cast<uint32_t>(RTC.bootCounter)));
  }
}

#  ifndef WEBSERVER_SYSINFO_MINIMAL

void handle_sysinfo_memory() {
  addTableSeparator(F("Memory"), 2, 3);

#   ifdef ESP32
  addRowLabelValue(LabelType::HEAP_SIZE);
  addRowLabelValue(LabelType::HEAP_MIN_FREE);
#   endif // ifdef ESP32

  int freeMem = ESP.getFreeHeap();
  addRowLabel(LabelType::FREE_MEM);
  {
    addHtmlInt(freeMem);
    addUnit(getFormUnit(LabelType::FREE_MEM));
#   ifndef BUILD_NO_RAM_TRACKER
    addHtml(F(" ("));
    addHtmlInt(lowestRAM);
    addHtml(F(" - "));
    addHtml(lowestRAMfunction);
    addHtml(')');
#   endif // ifndef BUILD_NO_RAM_TRACKER
  }
#   if defined(CORE_POST_2_5_0) || defined(ESP32)
 #    ifndef LIMIT_BUILD_SIZE
  addRowLabelValue(LabelType::HEAP_MAX_FREE_BLOCK);
 #    endif // ifndef LIMIT_BUILD_SIZE
#   endif   // if defined(CORE_POST_2_5_0) || defined(ESP32)
#   if defined(CORE_POST_2_5_0)
  #    ifndef LIMIT_BUILD_SIZE
  addRowLabelValue(LabelType::HEAP_FRAGMENTATION);
  #    endif // ifndef LIMIT_BUILD_SIZE
  {
    #    ifdef USE_SECOND_HEAP
    addRowLabelValue(LabelType::FREE_HEAP_IRAM);
    #    endif
  }
#   endif // if defined(CORE_POST_2_5_0)


  addRowLabel(LabelType::FREE_STACK);
  {
    addHtmlInt(getCurrentFreeStack());
    addUnit(getFormUnit(LabelType::FREE_STACK));
#   ifndef BUILD_NO_RAM_TRACKER
    addHtml(F(" ("));
    addHtmlInt(lowestFreeStack);
    addHtml(F(" - "));
    addHtml(lowestFreeStackfunction);
    addHtml(')');
#   endif // ifndef BUILD_NO_RAM_TRACKER
  }

#   if defined(ESP32) && defined(BOARD_HAS_PSRAM)

  addRowLabelValue(LabelType::PSRAM_SIZE);

  if (UsePSRAM()) {
    static const LabelType::Enum labels[] PROGMEM =
    {
      LabelType::PSRAM_FREE,
      LabelType::PSRAM_MIN_FREE,
      LabelType::PSRAM_MAX_FREE_BLOCK,
      LabelType::MAX_LABEL
    };
    addRowLabelValues(labels);
  }
#   endif // if defined(ESP32) && defined(BOARD_HAS_PSRAM)
}

#  endif // ifndef WEBSERVER_SYSINFO_MINIMAL

#  if FEATURE_ETHERNET

void handle_sysinfo_Ethernet() {
  if (active_network_medium == ESPEasy::net::NetworkMedium_t::Ethernet) {
    addTableSeparator(F("Ethernet"), 2, 3);

    static const LabelType::Enum labels[] PROGMEM =
    {
      LabelType::ETH_CHIP,
      LabelType::ETH_STATE,
      LabelType::ETH_SPEED,
      LabelType::ETH_DUPLEX,
      LabelType::ETH_MAC,

      //    LabelType::ETH_IP_ADDRESS_SUBNET,
      //    LabelType::ETH_IP_GATEWAY,
      //    LabelType::ETH_IP_DNS,

      LabelType::MAX_LABEL
    };

    addRowLabelValues(labels);

  }
}

#  endif // if FEATURE_ETHERNET

void handle_sysinfo_Network() {
  addTableSeparator(F("Network"), 2, 3);

  {
    static const LabelType::Enum labels[] PROGMEM =
    {
#  if FEATURE_ETHERNET || defined(USES_ESPEASY_NOW)
      LabelType::ETH_WIFI_MODE,
#  endif
      LabelType::IP_CONFIG,
      LabelType::IP_ADDRESS_SUBNET,
#  if FEATURE_USE_IPV6
      LabelType::IP6_LOCAL,
      LabelType::IP6_GLOBAL,

      //      LabelType::IP6_ALL_ADDRESSES,
#  endif // if FEATURE_USE_IPV6
      LabelType::GATEWAY,
      LabelType::CLIENT_IP,
      LabelType::DNS,
      LabelType::ALLOWED_IP_RANGE,
      LabelType::CONNECTED,
      LabelType::NUMBER_RECONNECTS,

      LabelType::MAX_LABEL
    };

    addRowLabelValues(labels);
  }

  addTableSeparator(F("WiFi"), 2, 3, F("Wifi"));

  const bool showWiFiConnectionInfo = ESPEasyWiFi.connected();

  addRowLabel(LabelType::WIFI_CONNECTION);

  if (showWiFiConnectionInfo)
  {
    addHtml(strformat(
              F("%s (RSSI %d dBm)"),
              FsP(toString(ESPEasy::net::wifi::getConnectionProtocol())),
              WiFi.RSSI()));
  } else { addHtml('-'); }

  addRowLabel(LabelType::SSID);

  if (showWiFiConnectionInfo)
  {
    addHtml(WiFi.SSID());
    addHtml(F(" ("));
    addHtml(WiFi.BSSIDstr());
    addHtml(')');
  } else { addHtml('-'); }

  #  ifdef ESP32
  const int64_t tsf_time = ESPEasy::net::wifi::WiFi_get_TSF_time();

  if (tsf_time > 0) {
    addRowLabel(F("WiFi TSF time"));

    // Split it while printing, so we're not loosing a lot of decimals in the float conversion
    uint32_t tsf_usec{};
    addHtml(secondsToDayHourMinuteSecond(micros_to_sec_usec(tsf_time, tsf_usec)));
    addHtml(strformat(F(".%06u"), tsf_usec));
  }
  #  endif // ifdef ESP32

  addRowLabel(getLabel(LabelType::CHANNEL));

  if (showWiFiConnectionInfo) {
    addHtml(getValue(LabelType::CHANNEL));
#  if CONFIG_SOC_WIFI_SUPPORT_5G
    addHtml(WiFi.channel() < 36 ? F(" (2.4 GHz)") : F(" (5 GHz)"));
#  endif
  } else { addHtml('-'); }

  addRowLabel(getLabel(LabelType::ENCRYPTION_TYPE_STA));

  if (showWiFiConnectionInfo) {
    addHtml(getValue(LabelType::ENCRYPTION_TYPE_STA));
  } else { addHtml('-'); }

  if (active_network_medium == ESPEasy::net::NetworkMedium_t::WIFI)
  {
    addRowLabel(LabelType::LAST_DISCONNECT_REASON);
    addHtml(getValue(LabelType::LAST_DISC_REASON_STR));
    addRowLabelValue(LabelType::WIFI_STORED_SSID1);
    addRowLabelValue(LabelType::WIFI_STORED_SSID2);
  }

  addRowLabelValue(LabelType::STA_MAC);
  addRowLabelValue(LabelType::AP_MAC);
  html_TR();
}

#  ifndef WEBSERVER_SYSINFO_MINIMAL

void handle_sysinfo_WiFiSettings() {
  addTableSeparator(F("WiFi Settings"), 2, 3);

  static const LabelType::Enum labels[] PROGMEM =
  {
    LabelType::FORCE_WIFI_BG,
    LabelType::RESTART_WIFI_LOST_CONN,
    LabelType::FORCE_WIFI_NOSLEEP,
#   ifdef SUPPORT_ARP
    LabelType::PERIODICAL_GRAT_ARP,
#   endif // ifdef SUPPORT_ARP
    LabelType::CONNECTION_FAIL_THRESH,
#   if FEATURE_SET_WIFI_TX_PWR
    LabelType::WIFI_TX_MAX_PWR,
    LabelType::WIFI_CUR_TX_PWR,
    LabelType::WIFI_SENS_MARGIN,
    LabelType::WIFI_SEND_AT_MAX_TX_PWR,
#   endif // if FEATURE_SET_WIFI_TX_PWR
    LabelType::WIFI_NR_EXTRA_SCANS,
#   ifdef USES_ESPEASY_NOW
    LabelType::USE_ESPEASY_NOW,
    LabelType::FORCE_ESPEASY_NOW_CHANNEL,
#   endif // ifdef USES_ESPEASY_NOW
    LabelType::WIFI_USE_LAST_CONN_FROM_RTC,
#   ifndef ESP32
    LabelType::WAIT_WIFI_CONNECT,
#   endif
#   ifdef ESP32
    LabelType::WIFI_PASSIVE_SCAN,
#   endif
    LabelType::HIDDEN_SSID_SLOW_CONNECT,
    LabelType::CONNECT_HIDDEN_SSID,
    LabelType::SDK_WIFI_AUTORECONNECT,
#   if FEATURE_USE_IPV6
    LabelType::ENABLE_IPV6,
#   endif

    LabelType::MAX_LABEL
  };

  addRowLabelValues(labels);
}

#  endif // ifndef WEBSERVER_SYSINFO_MINIMAL

void handle_sysinfo_Firmware() {
  addTableSeparator(F("Firmware"), 2, 3);

  addRowLabelValue_copy(LabelType::BUILD_DESC);
  addHtml(' ');
  addHtml(F(BUILD_NOTES));

  addRowLabelValue_copy(LabelType::SYSTEM_LIBRARIES);
#  ifdef ESP32
  addRowLabelValue_copy(LabelType::ESP_IDF_SDK_VERSION);
#  endif
  addRowLabelValue_copy(LabelType::GIT_BUILD);
  addRowLabelValue_copy(LabelType::PLUGIN_COUNT);
  addHtml(' ');
  addHtml(getPluginDescriptionString());

  addRowLabel(F("Build Origin"));
  addHtml(get_build_origin());
  addRowLabelValue_copy(LabelType::BUILD_TIME);
  addRowLabelValue_copy(LabelType::BINARY_FILENAME);
  addRowLabelValue_copy(LabelType::BUILD_PLATFORM);
  addRowLabelValue_copy(LabelType::GIT_HEAD);
  #  ifdef CONFIGURATION_CODE
  addRowLabelValue_copy(LabelType::CONFIGURATION_CODE_LBL);
  #  endif  // ifdef CONFIGURATION_CODE
}

#  ifndef WEBSERVER_SYSINFO_MINIMAL

void handle_sysinfo_SystemStatus() {
  addTableSeparator(F("System Status"), 2, 3);

  static const LabelType::Enum labels[] PROGMEM =
  {
    // Actual Loglevel
    LabelType::SYSLOG_LOG_LEVEL,
    LabelType::SERIAL_LOG_LEVEL,
    LabelType::WEB_LOG_LEVEL,
#   if FEATURE_SD
    LabelType::SD_LOG_LEVEL,
#   endif // if FEATURE_SD

    LabelType::ENABLE_SERIAL_PORT_CONSOLE,
    LabelType::CONSOLE_SERIAL_PORT,
#   if USES_ESPEASY_CONSOLE_FALLBACK_PORT
    LabelType::CONSOLE_FALLBACK_TO_SERIAL0,
    LabelType::CONSOLE_FALLBACK_PORT,
#   endif // if USES_ESPEASY_CONSOLE_FALLBACK_PORT
    LabelType::MAX_LABEL
  };

  addRowLabelValues(labels);
#   if FEATURE_CLEAR_I2C_STUCK

  if (Settings.EnableClearHangingI2Cbus()) {
    addRowLabelValue(LabelType::I2C_BUS_STATE);
    addRowLabelValue(LabelType::I2C_BUS_CLEARED_COUNT);
  }
#   endif // if FEATURE_CLEAR_I2C_STUCK
}

#  endif // ifndef WEBSERVER_SYSINFO_MINIMAL

#  ifndef WEBSERVER_SYSINFO_MINIMAL

void handle_sysinfo_NetworkServices() {
  addTableSeparator(F("Network Services"), 2, 3);

  addRowLabel(F("Network Connected"));
  addEnabled(ESPEasy::net::NetworkConnected());

  addRowLabel(F("NTP Initialized"));
  addEnabled(statusNTPInitialized);

  #   if FEATURE_MQTT

  if (validControllerIndex(firstEnabledMQTT_ControllerIndex())) {
    addRowLabel(F("MQTT Client Connected"));
    addEnabled(MQTTclient_connected);
  }
  #   endif // if FEATURE_MQTT
}

#  endif // ifndef WEBSERVER_SYSINFO_MINIMAL

#  ifndef WEBSERVER_SYSINFO_MINIMAL

void handle_sysinfo_ESP_Board() {
  addTableSeparator(F("ESP Board"), 2, 3);


  addRowLabel(LabelType::ESP_CHIP_ID);

  const uint32_t chipID = getChipId();
  addHtml(strformat(
            F("%d (%s)"),
            chipID,
            formatToHex(chipID, 6).c_str()));

  static const LabelType::Enum labels[] PROGMEM =
  {
    LabelType::ESP_CHIP_FREQ,
#   ifdef ESP32
    LabelType::ESP_CHIP_XTAL_FREQ,
    LabelType::ESP_CHIP_APB_FREQ,
#   endif // ifdef ESP32

    LabelType::ESP_CHIP_MODEL,
#   if defined(ESP32)
    LabelType::ESP_CHIP_REVISION,
#   endif // if defined(ESP32)
    LabelType::ESP_CHIP_CORES,
    LabelType::BOARD_NAME,
    LabelType::MAX_LABEL
  };

  addRowLabelValues(labels);


#   if defined(ESP32)
  addRowLabel(F("ESP Chip Features"));
  addHtml(getChipFeaturesString());
#   endif // if defined(ESP32)
}

#  endif // ifndef WEBSERVER_SYSINFO_MINIMAL

#  ifndef WEBSERVER_SYSINFO_MINIMAL

void handle_sysinfo_Storage() {
  addTableSeparator(F("Storage"), 2, 3);

  if (getFlashChipId() != 0) {
    addRowLabel(LabelType::FLASH_CHIP_ID);


    // Set to HEX may be something like 0x1640E0.
    // Where manufacturer is 0xE0 and device is 0x4016.
    addHtml(F("Vendor: "));
    addHtml(getValue(LabelType::FLASH_CHIP_VENDOR));

    if (flashChipVendorPuya())
    {
      addHtml(F(" (PUYA"));

      if (puyaSupport()) {
        addHtml(F(", supported"));
      } else {
        addHtml(F(HTML_SYMBOL_WARNING));
      }
      addHtml(')');
    }
    addHtml(F(" Device: "));
    addHtml(getValue(LabelType::FLASH_CHIP_MODEL));
    #   ifdef ESP32

    if (getChipFeatures().embeddedFlash) {
      addHtml(F(" (Embedded)"));
    }
    #   endif // ifdef ESP32
  }
  const uint32_t realSize = getFlashRealSizeInBytes();
  const uint32_t ideSize  = ESP.getFlashChipSize();

  addRowLabel(LabelType::FLASH_CHIP_REAL_SIZE);
  addHtmlInt(realSize / 1024);
  addHtml(F(" [kB]"));

  addRowLabel(LabelType::FLASH_IDE_SIZE);
  addHtmlInt(ideSize / 1024);
  addHtml(F(" [kB]"));

  addRowLabel(LabelType::FLASH_CHIP_SPEED);
  addHtmlInt(getFlashChipSpeed() / 1000000);
  addHtml(F(" [MHz]"));

  // Please check what is supported for the ESP32
  addRowLabel(LabelType::FLASH_IDE_SPEED);
  addHtmlInt(ESP.getFlashChipSpeed() / 1000000);
  addHtml(F(" [MHz]"));

  addRowLabelValue(LabelType::FLASH_IDE_MODE);

  addRowLabel(LabelType::FLASH_WRITE_COUNT);
  addHtml(strformat(
            F("%d daily / %d boot"),
            RTC.flashDayCounter,
            static_cast<int>(RTC.flashCounter)));

  {
    uint32_t maxSketchSize;
    bool     use2step;
    #   if defined(ESP8266)
    bool otaEnabled =
    #   endif // if defined(ESP8266)
    OTA_possible(maxSketchSize, use2step);

    addRowLabel(LabelType::SKETCH_SIZE);
    addHtml(strformat(
              F("%d [kB] (%d kB not used)"),
              getSketchSize() / 1024,
              (maxSketchSize - getSketchSize()) / 1024));

    addRowLabel(LabelType::MAX_OTA_SKETCH_SIZE);
    addHtml(strformat(
              F("%d [kB] (%d bytes)"),
              maxSketchSize / 1024,
              maxSketchSize));

    #   if defined(ESP8266)
    addRowLabel(LabelType::OTA_POSSIBLE);
    addHtml(boolToString(otaEnabled));

    addRowLabel(LabelType::OTA_2STEP);
    addHtml(boolToString(use2step));
    #   endif // if defined(ESP8266)
  }

  addRowLabel(LabelType::FS_SIZE);
  addHtml(strformat(
            F("%d [kB] (%d kB free)"),
            SpiffsTotalBytes() / 1024,
            SpiffsFreeSpace() / 1024));

  #   ifndef LIMIT_BUILD_SIZE
  addRowLabel(F("Page size"));
  addHtmlInt(SpiffsPagesize());
  addUnit(getFormUnit(LabelType::FREE_MEM)); // FIXME TD-er: Add labeltype

  addRowLabel(F("Block size"));
  addHtmlInt(SpiffsBlocksize());
  addUnit(getFormUnit(LabelType::FREE_MEM)); // FIXME TD-er: Add labeltype

  addRowLabel(F("Number of blocks"));
  addHtmlInt(SpiffsTotalBytes() / SpiffsBlocksize());

  {
  #    if defined(ESP8266)
    fs::FSInfo fs_info;
    ESPEASY_FS.info(fs_info);
    addRowLabel(F("Maximum open files"));
    addHtmlInt(fs_info.maxOpenFiles);

    addRowLabel(F("Maximum path length"));
    addHtmlInt(fs_info.maxPathLength);

  #    endif // if defined(ESP8266)
  }
  #   endif // ifndef LIMIT_BUILD_SIZE

#   if FEATURE_CHART_STORAGE_LAYOUT

  if (showSettingsFileLayout) {
    addTableSeparator(F("Settings Files"), 2, 3);
    html_TR_TD();
    addHtml(F("Layout Settings File"));
    html_TD();
    getConfig_dat_file_layout();
    html_TR_TD();
    html_TD();
    addHtml(F("(offset / size per item / index)"));

    for (int st = 0; st < static_cast<int>(SettingsType::Enum::SettingsType_MAX); ++st) {
      const SettingsType::Enum settingsType = static_cast<SettingsType::Enum>(st);
      #    if !FEATURE_NOTIFIER

      if (settingsType == SettingsType::Enum::NotificationSettings_Type) {
        continue;
      }
      #    endif // if !FEATURE_NOTIFIER
      html_TR_TD();
      addHtml(SettingsType::getSettingsTypeString(settingsType));
      html_BR();
      addHtml(SettingsType::getSettingsFileName(settingsType));
      html_TD();
      getStorageTableSVG(settingsType);
      delay(1);
    }
  }

  #    ifdef ESP32
  addTableSeparator(F("Partitions"), 2, 3,
                    F("https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/partition-tables.html"));

  addRowLabel(F("Data Partition Table"));

  //   TXBuffer += getPartitionTableHeader(F(" - "), F("<BR>"));
  //   TXBuffer += getPartitionTable(ESP_PARTITION_TYPE_DATA, F(" - "), F("<BR>"));
  getPartitionTableSVG(ESP_PARTITION_TYPE_DATA, 0x5856e6);

  addRowLabel(F("App Partition Table"));

  //   TXBuffer += getPartitionTableHeader(F(" - "), F("<BR>"));
  //   TXBuffer += getPartitionTable(ESP_PARTITION_TYPE_APP , F(" - "), F("<BR>"));
  getPartitionTableSVG(ESP_PARTITION_TYPE_APP, 0xab56e6);
  #    endif // ifdef ESP32

  #    ifdef ESP8266
  addTableSeparator(F("Partitions"), 2, 3);

  addRowLabel(F("Partition Table"));

  getPartitionTableSVG();
  #    endif // ifdef ESP8266
#   endif // if FEATURE_CHART_STORAGE_LAYOUT
}

#  endif // ifndef WEBSERVER_SYSINFO_MINIMAL

# endif    // ifdef WEBSERVER_SYSINFO


#endif // if defined(WEBSERVER_SYSINFO) || SHOW_SYSINFO_JSON
