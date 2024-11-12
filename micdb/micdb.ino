/**
 * @file mic_decibel.ino
 * @brief M5StickCPlus2 Microphone Decibel Display Test
 * @version 0.1
 * @date 2023-12-19
 *
 * @Hardwares: M5StickCPlus2
 * @Platform Version: Arduino M5Stack Board Manager v2.0.9
 * @Dependent Library:
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5Unified: https://github.com/m5stack/M5Unified
 */


/* TODO
- Record Timer
- FFT?
- Online analysis?
*/

#include <M5StickCPlus2.h>
#include <LittleFS.h>
#include "FS.h"

static constexpr const size_t buffer_length = 4410;
static constexpr const size_t sample_rate = 44100;
static int16_t *audio_buffer;
static char filename[17];

static m5::rtc_datetime_t DATE;

static uint32_t min_timer = 3;
//static uint8_t min;

/* TFT_state
0 - off (blink LED)
1 - on (low b)
2 - on (high b) 
*/
static uint8_t TFT_state = 1;

/* STATE
0 - menu
1 - rec
*/
static uint8_t state = 0;

#define PIN_CLK  0
#define PIN_DATA 34

#define FORMAT_LITTLEFS_IF_FAILED true

#define ST "start" // Start string in log file


float grabDB(){
  if (StickCP2.Mic.isEnabled()) {
    if (StickCP2.Mic.record(audio_buffer, buffer_length, sample_rate)) {
      // Calculate RMS (Root Mean Square) for the buffer to determine dB level
      float rms = 0;
      for (size_t i = 0; i < buffer_length; i++) {
          rms += audio_buffer[i] * audio_buffer[i];
      }
      rms = sqrt(rms / buffer_length);

      // Convert RMS to decibels
      return 20 * log10(rms);
    }
  }
}


void recording(){
    float decibel = grabDB();
    uint8_t ms = millis();

    File logFile = LittleFS.open(filename, FILE_APPEND);
    logFile.write((uint8_t*)&decibel, sizeof(decibel));
    logFile.write((uint8_t*)&ms, sizeof(ms));
    logFile.close();

    if (TFT_state) dispDB(decibel);
}

void dispDB(float decibel){
      // Display the decibel value
      StickCP2.Display.fillRect(60,65,110,20,TFT_BLACK);
      StickCP2.Display.setCursor(60, 65);
      StickCP2.Display.setFont(&fonts::Font4);
      StickCP2.Display.printf(" %.2f dB", decibel);
}

void rec_BtnA(){
}


void menu_BtnA(){
  if (StickCP2.BtnA.wasClicked()){
    //Init recording
    setup_rec();
    state = 1; // Change state to recording
    StickCP2.Display.clear();
    StickCP2.Display.drawString("Recording...", 120, 3);
  }

  if (StickCP2.BtnA.wasHold()) {

    int sec = 3;

    while(StickCP2.BtnA.isHolding() && (sec > 0)){
      StickCP2.Display.clear();
      StickCP2.Display.setCursor(10,60);
      StickCP2.Display.printf("Formatting in: %d s",sec);

      delay(1000);
      sec -= 1;
      StickCP2.update();
    }

    if (sec <= 0) {
      format(LittleFS);
      StickCP2.Display.drawString("Complete!", 120, 100);
      delay(3000);
    }

    wifiDisplay();
    
  } 
}

void menu_BtnB(){
    if (StickCP2.BtnB.wasClicked()) {
        TFT_state += (TFT_state == 2) ? -2 : 1 ;
        dispbset();
    }
    if (StickCP2.BtnB.wasHold()) {
      listDir(LittleFS);
      while(StickCP2.BtnB.isHolding()){ 
        delay(1000);
        StickCP2.update();
      }
      wifiDisplay();
    }
}

void init_log(){
    //Log File

    uint8_t year    = DATE.date.year - 2000;
    uint8_t month   = DATE.date.month;
    uint8_t day     = DATE.date.date;
    uint8_t hour    = DATE.time.hours;
    uint8_t minute  = DATE.time.minutes;
    uint8_t second  = DATE.time.seconds;

    // "/YYMMDD-HHMM.bin"
    snprintf(filename, sizeof(filename), "/%02d%02d%02d-%02d%02d.bin",
             year, month, day, hour, minute);

    File logFile = LittleFS.open(filename, FILE_WRITE);
    /*
    StickCP2.Display.setFont(&fonts::Font0);
    StickCP2.Display.setCursor(10, 100);  // Position the text
    StickCP2.Display.printf("Writing to: %s",filename);
    StickCP2.Display.setCursor(10, 110);  // Position the text
    StickCP2.Display.printf("Time: %d-%d-%d %d:%d:%d\n",
              year, month, day, hour, minute, second);
    StickCP2.Display.setFont(&fonts::FreeSansBoldOblique12pt7b);
    */
    
    if (!logFile){
      StickCP2.Display.clear();
      StickCP2.Display.drawString("Failed creating file!", 120, 3);
      Serial.println("Failed creating file!");
      while (true);  // Stop if LittleFS fails
    } 


    char a[] = ST;
    logFile.write((uint8_t*)&a, sizeof(a));
    logFile.write((uint8_t*)&year, sizeof(year));
    logFile.write((uint8_t*)&month, sizeof(month));
    logFile.write((uint8_t*)&day, sizeof(day));
    logFile.write((uint8_t*)&hour, sizeof(hour));
    logFile.write((uint8_t*)&minute, sizeof(minute));
    logFile.write((uint8_t*)&second, sizeof(second));
    logFile.close();
}

void setup_rec(){
  endserver();
  WiFi.mode(WIFI_OFF);
  btStop();
  init_log();
  //dispbset(1);
  //StickCP2.Display.writeCommand(TFT_DISPOFF);
}

void dispbset(const uint8_t ns){
  TFT_state = ns;
  dispbset();
};

// Set brightness of display based on global TFT_state
void dispbset(){
  uint8_t b = (TFT_state >= 2) ? 255 : TFT_state;
  StickCP2.Display.setBrightness(b);
  Serial.printf("Set brightness to %d\n",b);

  uint8_t lb = (TFT_state == 0) ? 10 : 0;
  StickCP2.Power.setLed(lb);
};



void setup(void) {
    auto cfg = M5.config();
    
    StickCP2.begin(cfg);
    StickCP2.Display.startWrite();
    StickCP2.Display.setRotation(1);
    StickCP2.Display.setTextDatum(top_center);
    StickCP2.Display.setTextColor(WHITE);
    StickCP2.Display.setBrightness(1);
    StickCP2.Display.setFont(&fonts::FreeSansBoldOblique12pt7b);

    audio_buffer = (int16_t*) heap_caps_malloc(buffer_length * sizeof(int16_t), MALLOC_CAP_8BIT);
    memset(audio_buffer, 0, buffer_length * sizeof(int16_t));

    //StickCP2.Speaker.setVolume(255);
    StickCP2.Speaker.end(); // Ensure speaker is off
    StickCP2.Mic.begin(); // Ensure mic is on

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
        .sample_rate = sample_rate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
#if ESP_IDF_VERSION > ESP_IDF_VERSION_VAL(4, 1, 0)
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
#else
        .communication_format = I2S_COMM_FORMAT_I2S,
#endif
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 3,
        .dma_buf_len = 256,
    };

    i2s_pin_config_t pin_config;
#if (ESP_IDF_VERSION > ESP_IDF_VERSION_VAL(4, 3, 0))
    pin_config.mck_io_num = I2S_PIN_NO_CHANGE;
#endif
    pin_config.bck_io_num   = I2S_PIN_NO_CHANGE;
    pin_config.ws_io_num    = PIN_CLK;
    pin_config.data_out_num = I2S_PIN_NO_CHANGE;
    pin_config.data_in_num  = PIN_DATA;

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    i2s_set_clk(I2S_NUM_0, sample_rate, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
    
    StickCP2.update();
    Serial.begin(115200);
    delay(500);
    DATE = StickCP2.Rtc.getDateTime();

    // Initialize LittleFS
    if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
        StickCP2.Display.drawString("Failed!", 120, 3);
        while (true);  // Stop if LittleFS fails
    }

    // Initialize Wifi
    syncRTC();
    setupwifi();
    startserver();
    wifiDisplay();
    //init_log();
}

void loop(void) {
    StickCP2.update();
    
    //Log File
    DATE = StickCP2.Rtc.getDateTime();

    switch (state) {
      case 0: { //menu
        // DISPLAYS
        dispBat();
        dispDB(grabDB());
        dispRTC();

        menu_BtnA();
        menu_BtnB();
        delay(100);
        break;
      }
      case 1:{ // rec
        //rec_BtnA();
        recording();
        StickCP2.Power.setLed(0);
        delay(900);
        StickCP2.Power.setLed(1);
        break;
      }
    }

}