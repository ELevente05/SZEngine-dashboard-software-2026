/*
 * ESP32-S3-N16R8 Receiver Dashboard
 */

#include <Arduino.h>
#include <U8g2lib.h>
#include <math.h>
#include "driver/twai.h"
#include <Adafruit_NeoPixel.h>

// --- PIN CONFIG ---
#define SPI_SCK   12
#define SPI_MOSI  11
#define SPI_CS    10
#define SPI_DC    9
#define SPI_RESET 8
#define BACKLIGHT_PIN 4

#define CAN_TX_PIN 47
#define CAN_RX_PIN 48
#define LED_PIN 7
#define NUM_LEDS 9

U8G2_UC1611_EA_DOGXL240_F_4W_SW_SPI u8g2(U8G2_R2, SPI_SCK, SPI_MOSI, SPI_CS, SPI_DC, SPI_RESET);
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// --- DYNAMIC DATA VARIABLES ---
int activeScreen = 1; 
float oilTemp = 85.0; 
float oilPress = 4.0; 
float engineWaterTemp = 88.0; 
float icWaterTemp = 35.0;
float lambdaVal = 1.00; 
float intakeTemp = 35.0; 
float egt = 450.0; 
float batteryVolts = 13.8;
float boostPressure = 0.0; 
float hybridTemp = 25.0; 
float hybridVolts = 36.0;
int currentGear = 0; 
int stateOfCharge = 80; 
int rpm = 0;

// --- SETTINGS & TIMERS ---
int rpmStart = 4000; 
int rpmMax = 10000;   
unsigned long lastScreenUpdate = 0; // Timer for 30FPS rendering

// --- HELPERS ---
int16_t parseBE(uint8_t* data, int offset) { return (data[offset] << 8) | data[offset + 1]; }

// --- SHIFT LIGHT LOGIC ---
void updateLEDs() {
  int numLedsToLight = 0;
  bool revLimiter = false;

  if (rpm >= rpmMax) {
    revLimiter = true;
  } else if (rpm >= rpmStart) {
    numLedsToLight = (int)((rpm - rpmStart) * NUM_LEDS / (float)(rpmMax - rpmStart)) + 1;
    if (numLedsToLight > NUM_LEDS) numLedsToLight = NUM_LEDS;
  }

  strip.clear(); 

  if (revLimiter) {
    // 10Hz Strobe effect for blue shift light
    if ((millis() / 50) % 2 == 0) {
      for(int i = 0; i < NUM_LEDS; i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 255));
      }
    }
  } else {
    // Smooth 11-step sweep
    for (int i = 0; i < NUM_LEDS; i++) {
      if (i >= NUM_LEDS - numLedsToLight) {
        if (i >= 6) strip.setPixelColor(i, strip.Color(0, 255, 0));       
        else if (i >= 3) strip.setPixelColor(i, strip.Color(255, 255, 0)); 
        else strip.setPixelColor(i, strip.Color(255, 0, 0));               
      }
    }
  }
  
  strip.show(); 
}

// --- SCREEN DRAWING ---
void drawGauge(int cx, int cy, int radius, int thickness, float minVal, float maxVal, float val) {
  if (val < minVal) val = minVal; 
  if (val > maxVal) val = maxVal;
  u8g2.drawCircle(cx, cy, radius); 
  u8g2.drawCircle(cx, cy, radius - thickness);
  float start_angle = 2.356; 
  float end_angle = 7.068;   
  float target_angle = start_angle + ((val - minVal) / (maxVal - minVal)) * (end_angle - start_angle);
  for (float a = start_angle; a <= target_angle; a += 0.05) {
    int x = cx + (radius - thickness/2) * cos(a);
    int y = cy + (radius - thickness/2) * sin(a);
    u8g2.drawDisc(x, y, thickness/2);
  }
}

void drawScreen1() {
  char textBuffer[32]; 
  u8g2.setFont(u8g2_font_logisoso92_tn); 
  snprintf(textBuffer, sizeof(textBuffer), "%d", currentGear);
  u8g2.drawStr(95, 125, textBuffer);
  
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);
  u8g2.setFont(u8g2_font_profont22_tr);
  u8g2.drawStr(20, 24, "Boost");
  u8g2.drawStr(105, 24, "Gear");
  u8g2.drawStr(190, 24, "SoC");
  u8g2.drawStr(161, 104, "Hy.T"); 

  u8g2.setFont(u8g2_font_profont29_tr);
  snprintf(textBuffer, sizeof(textBuffer), "%d%%", stateOfCharge); 
  u8g2.drawStr(176, 48, textBuffer);

  u8g2.setFont(u8g2_font_profont22_tr);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", boostPressure);
  u8g2.drawStr(33, 77, textBuffer);

  u8g2.setFont(u8g2_font_profont22_tf);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f°C", hybridTemp);
  u8g2.drawUTF8(161, 126, textBuffer); 

  drawGauge(50, 70, 40, 10, 0.0, 2.5, boostPressure);
}

void drawScreen2() {
  char textBuffer[16];
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);
  u8g2.drawLine(120, 0, 120, 128);
  u8g2.drawLine(0, 42, 240, 42);
  u8g2.drawLine(0, 84, 240, 84);
  u8g2.drawLine(60, 0, 60, 128);
  u8g2.drawLine(180, 0, 180, 128);

  u8g2.setFont(u8g2_font_t0_16b_tr);
  u8g2.drawStr(8, 11, "Oil T");
  u8g2.drawStr(68, 11, "Oil P");
  u8g2.drawStr(122, 11, "EWaterT");
  u8g2.drawStr(183, 12, "IWaterT");

  u8g2.setFont(u8g2_font_profont22_tr);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", oilTemp); u8g2.drawStr(5, 35, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", oilPress); u8g2.drawStr(65, 35, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", engineWaterTemp); u8g2.drawStr(125, 35, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", icWaterTemp); u8g2.drawStr(187, 35, textBuffer);

  u8g2.setFont(u8g2_font_t0_16b_tr);
  u8g2.drawStr(4, 56, "Lambda");
  u8g2.drawStr(61, 56, "IntakeT");
  u8g2.drawStr(135, 56, "EGT");
  u8g2.drawStr(183, 56, "Battery");

  u8g2.setFont(u8g2_font_profont22_tr);
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", lambdaVal); u8g2.drawStr(4, 77, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", intakeTemp); u8g2.drawStr(62, 77, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", egt); u8g2.drawStr(121, 77, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", batteryVolts); u8g2.drawStr(182, 77, textBuffer);

  u8g2.setFont(u8g2_font_t0_16b_tr);
  u8g2.drawStr(4, 99, "Boost");
  u8g2.drawStr(62, 99, "HybridT");
  u8g2.drawStr(123, 99, "HybridV");
  u8g2.drawStr(195, 99, "Gear");

  u8g2.setFont(u8g2_font_profont22_tr);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", boostPressure); u8g2.drawStr(4, 120, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", hybridTemp); u8g2.drawStr(62, 120, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", hybridVolts); u8g2.drawStr(121, 120, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%d", currentGear); u8g2.drawStr(205, 120, textBuffer);
}

void setup() {
  // Serial is kept strictly for hardware debugging (e.g. Serial.println), but it no longer pauses the loop
  Serial.begin(115200); 
  delay(1000); 
  
  pinMode(BACKLIGHT_PIN, OUTPUT); 
  digitalWrite(BACKLIGHT_PIN, HIGH); 
  
  strip.begin(); 
  strip.setBrightness(150); 
  strip.clear(); 
  strip.show();
  
  u8g2.begin(); 
  u8g2.setContrast(150); 
  
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_NORMAL);
  g_config.rx_queue_len = 20; 
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); 
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  
  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    twai_start();
    Serial.println("CAN Started Successfully!");
  }
}

void loop() {
  // --- 1. NON-BLOCKING CAN INGESTION ---
  // The processor will instantly drain the queue as fast as messages arrive.
  twai_message_t rx_msg;
  while (twai_receive(&rx_msg, 0) == ESP_OK) {
    switch (rx_msg.identifier) {
      case 0x520: 
        rpm = parseBE(rx_msg.data, 0); 
        lambdaVal = parseBE(rx_msg.data, 6) * 0.001; 
        break;
      case 0x530: 
        batteryVolts = parseBE(rx_msg.data, 0) * 0.01; 
        intakeTemp = parseBE(rx_msg.data, 4) * 0.1; 
        engineWaterTemp = parseBE(rx_msg.data, 6) * 0.1; 
        break;
      case 0x531: 
        egt = parseBE(rx_msg.data, 6) * 1.0; 
        break;
      case 0x536: 
        currentGear = rx_msg.data[0]; 
        oilPress = parseBE(rx_msg.data, 4) * 0.001; 
        oilTemp = parseBE(rx_msg.data, 6) * 0.1; 
        break;
      case 0x101: 
        memcpy(&icWaterTemp, &rx_msg.data[4], 4); 
        break;
      case 0x104: 
        memcpy(&boostPressure, &rx_msg.data[0], 4); 
        memcpy(&hybridTemp, &rx_msg.data[4], 4); 
        break;
      case 0x105: 
        memcpy(&hybridVolts, &rx_msg.data[0], 4); 
        stateOfCharge = rx_msg.data[5]; 
        activeScreen = rx_msg.data[6]; 
        break;
    }
  }

  // --- 2. DECOUPLED 30 FPS RENDER ENGINE ---
  // 33 milliseconds = ~30 Frames Per Second. 
  // Everything inside this block only runs every 33ms.
  if (millis() - lastScreenUpdate >= 33) {
    lastScreenUpdate = millis();

    updateLEDs();
    u8g2.clearBuffer();          
    if (activeScreen == 1) {
      drawScreen1(); 
    } else if (activeScreen == 2) {
      drawScreen2();
    }
    u8g2.sendBuffer();          
  }
}