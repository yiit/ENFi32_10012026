#include "../WebServer/CacheControllerPages.h"

#ifdef USES_C016

# include "../WebServer/ESPEasy_WebServer.h"
# include "../WebServer/AccessControl.h"
# include "../WebServer/HTML_wrappers.h"
# include "../WebServer/JSON.h"
# include "../CustomBuild/ESPEasyLimits.h"
# include "../DataStructs/DeviceStruct.h"
# include "../DataStructs/ESPEasyControllerCache_CSV_dumper.h"
# include "../DataTypes/TaskIndex.h"
# include "../Globals/C016_ControllerCache.h"
# include "../Globals/Cache.h"
# include "../Globals/ESPEasy_time.h"
# include "../Globals/Settings.h"
# include "../Helpers/ESPEasy_math.h"
# include "../Helpers/ESPEasy_Storage.h"
# include "../Helpers/ESPEasy_time_calc.h"
# include "../Helpers/Misc.h"

// ********************************************************************************
// URLs needed for C016_CacheController
// to help dump the content of the binary log files
// ********************************************************************************
void handle_dumpcache() {
  if (!isLoggedIn()) { return; }

  // Filters/export settings
  char separator     = ';';
  bool joinTimestamp = false;
  bool onlySetTasks  = false;


  if (hasArg(F("separator"))) {
    String sep = webArg(F("separator"));

    if (isWrappedWithQuotes(sep)) {
      removeChar(sep, sep[0]);
    }

    if (sep.equalsIgnoreCase(F("Tab"))) { separator = '\t'; }
    else if (sep.equalsIgnoreCase(F("Comma"))) { separator = ','; }
    else if (sep.equalsIgnoreCase(F("Semicolon"))) { separator = ';'; }
  }

  if (hasArg(F("jointimestamp"))) {
    joinTimestamp = true;
  }

  if (hasArg(F("onlysettasks"))) {
    onlySetTasks = true;
  }

  {
    // Send HTTP headers to directly save the dump as a CSV file
    String str =  F("attachment; filename=cachedump_");
    str += Settings.getName();
    str += F("_U");
    str += Settings.Unit;

    if (node_time.systemTimePresent())
    {
      str += '_';
      str += node_time.getDateTimeString('\0', '\0', '\0');
    }
    str += F(".csv");

    sendHeader(F("Content-Disposition"), str);
    TXBuffer.startStream(F("application/octet-stream"), F("*"), 200);
  }


  ESPEasyControllerCache_CSV_dumper dumper(
    joinTimestamp,
    onlySetTasks,
    separator,
    ESPEasyControllerCache_CSV_dumper::Target::CSV_file);

  dumper.generateCSVHeader(true);

  while (dumper.createCSVLine()) {
    dumper.writeCSVLine(true);
  }

  TXBuffer.endStream();
}

void handle_cache_json() {
  if (!isLoggedIn()) { return; }

  // Flush any data still in RTC memory to the cache files.
  C016_flush();

  TXBuffer.startJsonStream();
  {
    KeyValueWriter_JSON writer(true);
    {
      KeyValueWriter_JSON columnWriter(F("columns"), &writer);
      columnWriter.setIsArray();

      //     addHtml(F("UNIX timestamp;contr. idx;sensortype;taskindex;value count"));
      columnWriter.write({ F(""), F("UNIX timestamp") });
      columnWriter.write({ F(""), F("UTC timestamp") });
      columnWriter.write({ F(""), F("task index") });

      if (hasArg(F("pluginID"))) {
        columnWriter.write({ F(""), F("plugin ID") });
      }

      for (taskIndex_t i = 0; i < TASKS_MAX; ++i) {
        for (int j = 0; j < VARS_PER_TASK; ++j) {
          columnWriter.write({ F(""), strformat(
                                 F("%s#%s"),
                                 getTaskDeviceName(i).c_str(),
                                 getTaskValueName(i, j).c_str()) });
        }
      }
    }
    int fileCount = 0;
    {
      KeyValueWriter_JSON filesWriter(F("files"), &writer);
      filesWriter.setIsArray();
      bool islast = false;
      int  filenr = 0;

      while (!islast) {
        const String currentFile = C016_getCacheFileName(filenr, islast);
        ++filenr;

        if (currentFile.length() > 0) {
          ++fileCount;
          filesWriter.write({ F(""), currentFile });
        }
      }
    }
    {
      KeyValueWriter_JSON pluginIDWriter(F("pluginID"), &writer);
      pluginIDWriter.setIsArray();

      for (taskIndex_t taskIndex = 0; validTaskIndex(taskIndex); ++taskIndex) {
        pluginIDWriter.write({ F(""), getPluginID_from_TaskIndex(taskIndex).value });
      }
    }
    writer.write({ F("separator"), F(";") });
    writer.write({ F("nrfiles"), fileCount });
  }
  TXBuffer.endStream();
}

void handle_cache_csv() { if (!isLoggedIn()) { return; }
}

#endif // ifdef USES_C016
