#include "../WebServer/Log.h"

#include "../WebServer/ESPEasy_WebServer.h"
#include "../WebServer/404.h"
#include "../WebServer/HTML_wrappers.h"
#include "../WebServer/JSON.h"
#include "../WebServer/Markup.h"
#include "../WebServer/Markup_Buttons.h"

#include "../DataStructs/LogStruct.h"

#include "../Globals/Logging.h"
#include "../Globals/Settings.h"
#include "../Helpers/ESPEasy_time_calc.h"
#include "../Static/WebStaticData.h"

// ********************************************************************************
// Web Interface log page
// ********************************************************************************
void handle_log() {
  if (!isLoggedIn()) { return; }
  navMenuIndex = MENU_INDEX_TOOLS;

  TXBuffer.startStream();
  sendHeadandTail_stdtemplate(_HEAD);
  html_table_class_normal();

  #ifdef WEBSERVER_LOG
  addHtml(F("<TR><TH id=\"headline\" align=\"left\">Log"));
  addCopyButton(F("copyText"), EMPTY_STRING, F("Copy log to clipboard"));
  addHtml(F(
            "</TR></table><div  id='current_loglevel' style='font-weight: bold;'>Logging: </div><div class='logviewer' id='copyText_1'></div>"));
  addHtml(F("Autoscroll: "));
  addCheckBox(F("autoscroll"), true);
  addHtml(F("<BR></body>"));

  serve_JS(JSfiles_e::FetchAndParseLog);

  #else // ifdef WEBSERVER_LOG
  addHtml(F("Not included in build"));
  #endif // ifdef WEBSERVER_LOG
  sendHeadandTail_stdtemplate(_TAIL);
  TXBuffer.endStream();
}

// ********************************************************************************
// Web Interface JSON log page
// ********************************************************************************
void handle_log_JSON() {
  if (!isLoggedIn()) { return; }
  #ifdef WEBSERVER_LOG
  TXBuffer.startJsonStream();
  {
    KeyValueWriter_JSON top(true);
    {
      String webrequest = webArg(F("view"));
      KeyValueWriter_JSON mainWriter(F("Log"), &top);

      if (equals(webrequest, F("legend"))) {
        KeyValueWriter_JSON legendWriter(F("Legend"), &mainWriter);
        legendWriter.setIsArray();

        for (uint8_t i = 0; i < LOG_LEVEL_NRELEMENTS; ++i) {
          KeyValueWriter_JSON loglevelWriter(&legendWriter);
          int loglevel;
          loglevelWriter.write({ F("label"), getLogLevelDisplayStringFromIndex(i, loglevel) });
          loglevelWriter.write({ F("loglevel"), loglevel });
        }
      }
      unsigned long firstTimeStamp = 0;
      unsigned long lastTimeStamp  = 0;
      int nrEntries                = 0;

      {
        KeyValueWriter_JSON entriesWriter(F("Entries"), &mainWriter);
        entriesWriter.setIsArray();
        bool logLinesAvailable = true;

        while (logLinesAvailable) {
          String  message;
          uint8_t loglevel;

          if (Logging.getNext(logLinesAvailable, lastTimeStamp, message, loglevel)) {
            KeyValueWriter_JSON logWriter(&entriesWriter);
            logWriter.write({ F("timestamp"), lastTimeStamp });
            logWriter.write({ F("text"),      std::move(message) });
            logWriter.write({ F("level"), loglevel });

            if (nrEntries == 0) {
              firstTimeStamp = lastTimeStamp;
            }
            ++nrEntries;
          }

          // Do we need to do something here and maybe limit number of lines at once?
        }
      }
      long logTimeSpan       = timeDiff(firstTimeStamp, lastTimeStamp);
      long refreshSuggestion = 1000;
      long newOptimum        = 1000;

      if ((nrEntries > 2) && (logTimeSpan > 1)) {
        // May need to lower the TTL for refresh when time needed
        // to fill half the log is lower than current TTL
        newOptimum = logTimeSpan * (LOG_STRUCT_MESSAGE_LINES / 2);
        newOptimum = newOptimum / (nrEntries - 1);
      }

      if (newOptimum < refreshSuggestion) { refreshSuggestion = newOptimum; }

      if (refreshSuggestion < 100) {
        // Reload times no lower than 100 msec.
        refreshSuggestion = 100;
      }
      mainWriter.write({ F("TTL"),                 refreshSuggestion });
      mainWriter.write({ F("timeHalfBuffer"),      newOptimum });
      mainWriter.write({ F("nrEntries"),           nrEntries });
      mainWriter.write({ F("SettingsWebLogLevel"), Settings.WebLogLevel });
      mainWriter.write({ F("logTimeSpan"), logTimeSpan });
    }
  }
  TXBuffer.endStream();
  updateLogLevelCache();

  #else // ifdef WEBSERVER_LOG
  handleNotFound();
  #endif // ifdef WEBSERVER_LOG
}
