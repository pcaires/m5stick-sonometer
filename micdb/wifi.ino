#if defined(ARDUINO)
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#define NTP_TIMEZONE  "UTC-0"
#define NTP_SERVER1   "0.pool.ntp.org"
#define NTP_SERVER2   "1.pool.ntp.org"
#define NTP_SERVER3   "2.pool.ntp.org"

#if __has_include(<esp_sntp.h>)
#include <esp_sntp.h>
#define SNTP_ENABLED 1
#elif __has_include(<sntp.h>)
#include <sntp.h>
#define SNTP_ENABLED 1
#endif

#endif


#ifndef SNTP_ENABLED
#define SNTP_ENABLED 0
#endif

static AsyncWebServer server(80);
static IPAddress IP;

void setupwifi(){
  WiFi.softAP(SSID, PWD);
  IP = WiFi.softAPIP();

  // Route to list files
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<html><head><title>ESP32 File Server</title></head><body>";
    html += "<h2>File List:</h2><ul>";

    // List files with clickable links
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    while (file) {
        String fileName = file.name();
        html += "<li><a href=\"/download?file=/" + fileName + "\">" + fileName + "</a></li>";
        file = root.openNextFile();
    }
    html += "</ul></body></html>";

    request->send(200, "text/html", html);
  });

  // Route to download a specific file
  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("file")) {
      String filename = request->getParam("file")->value();
      if (LittleFS.exists(filename)) {
        request->send(LittleFS, filename, "application/octet-stream");
      } else {
        request->send(404, "text/plain", "File not found");
      }
    } else {
      request->send(400, "text/plain", "File parameter missing");
    }
  });
}

void startserver(){
  server.begin();
  Serial.println("Server started");
  wifiDisplay();
}

void endserver(){
  server.end();
  Serial.println("Server ended");
}

void wifiDisplay(){
  Serial.println("SSID: " SSID);
  Serial.println("PASS: " PWD);
  Serial.print("Access Point IP: ");
  Serial.println(IP);

  StickCP2.Display.clear();
  StickCP2.Display.setFont(&fonts::FreeMonoBold9pt7b);
  StickCP2.Display.setCursor(3,3);
  StickCP2.Display.println(SSID);
  StickCP2.Display.println("---");
  StickCP2.Display.println(PWD);
  StickCP2.Display.print(IP);
  
  StickCP2.Display.println("---");
  auto DATE = StickCP2.Rtc.getDateTime();

  uint8_t year    = DATE.date.year - 2000;
  uint8_t month   = DATE.date.month;
  uint8_t day     = DATE.date.date;
  uint8_t hour    = DATE.time.hours;
  uint8_t minute  = DATE.time.minutes;
  uint8_t second  = DATE.time.seconds;

  StickCP2.Display.printf("%04d-%02d-%02d Time: %02d:%02d:%02d\n",
                2000 + year, month, day,
                hour, minute, second);
  
}

void syncRTC(void) {

    StickCP2.Display.setTextColor(GREEN);

    if (!StickCP2.Rtc.isEnabled()) {
        Serial.println("RTC not found.");
        StickCP2.Display.println("RTC not found.");
        for (;;) {
            vTaskDelay(500);
        }
    }

    Serial.println("RTC found.");

    // It is recommended to set UTC for the RTC and ESP32 internal clocks.
    /* /// setup RTC ( direct setting )
      //                      YYYY  MM  DD      hh  mm  ss
      StickCP2.Rtc.setDateTime( { { 2021, 12, 31 }, { 12, 34, 56 } } );

    //*/

    /// setup RTC ( NTP auto setting )

    StickCP2.Display.print("WiFi:");
    WiFi.begin(SYNC_SSID, SYNC_PWD);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(500);
    }
    Serial.println("\r\n WiFi Connected.");
    StickCP2.Display.print("Connected.");

    configTzTime(NTP_TIMEZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);

#if SNTP_ENABLED
    while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
        Serial.print('.');
        delay(1000);
    }
#else
    delay(1600);
    struct tm timeInfo;
    while (!getLocalTime(&timeInfo, 1000)) {
        Serial.print('.');
    };
#endif

    Serial.println("\r\n NTP Connected.");

    time_t t = time(nullptr) + 1;  // Advance one second.
    while (t > time(nullptr))
        ;  /// Synchronization in seconds

    StickCP2.Rtc.setDateTime(gmtime(&t));
    StickCP2.Display.print("RTC Set");

    delay(2000);

    StickCP2.Display.setTextColor(WHITE);
    StickCP2.Display.clear();
}

