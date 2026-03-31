/*
 * ESP32-S3-N16R8 Receiver Dashboard
 * MaxxECU CAN Parsing & Adafruit NeoPixel F1 Shift Light
 * Optimized for 20Hz Telemetry
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

// --- LED CONFIG ---
#define LED_PIN 7
#define NUM_LEDS 9

U8G2_UC1611_EA_DOGXL240_F_4W_SW_SPI u8g2(U8G2_R2, SPI_SCK, SPI_MOSI, SPI_CS, SPI_DC, SPI_RESET);
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// --- DISPLAY STATE ---
int activeScreen = 1; 

// --- DYNAMIC DATA VARIABLES ---
float oilTemp = 85.5;
float oilPress = 4.2;
float engineWaterTemp = 88.0;
float icWaterTemp = 77.0;
float lambdaVal = 0.98;
float intakeTemp = 35.0;
float egt = 450.0;
float batteryVolts = 13.8;
float boostPressure = 0.0;
float hybridTemp = 25.0;
float hybridVolts = 36.0;
int currentGear = 0;
int stateOfCharge = 80;
int rpm = 0;

// --- SHIFT LIGHT SETTINGS ---
int rpmStart = 4000; 
int rpmMax = 7000;   

// --- MAXXECU BIG-ENDIAN PARSER ---
int16_t parseBE(uint8_t* data, int offset) {
  return (data[offset] << 8) | data[offset + 1];
}

// --- F1 SHIFT LIGHT LOGIC (NEOPIXEL) ---
void updateLEDs() {
  int numLedsToLight = map(rpm, rpmStart, rpmMax, 0, NUM_LEDS);
  if (numLedsToLight < 0) numLedsToLight = 0;
  if (numLedsToLight > NUM_LEDS) numLedsToLight = NUM_LEDS;

  strip.clear(); 

  // Rev Limiter Strobe (Blue)
  if (rpm >= rpmMax) {
    if ((millis() / 50) % 2 == 0) {
      for(int i=0; i<NUM_LEDS; i++) strip.setPixelColor(i, strip.Color(0, 0, 255)); 
    }
  } 
  // Normal F1 Sweep
  else {
    for (int i = 0; i < numLedsToLight; i++) {
      if (i < 3) strip.setPixelColor(i, strip.Color(0, 255, 0));       // Green
      else if (i < 6) strip.setPixelColor(i, strip.Color(255, 255, 0)); // Yellow
      else if (i < 8) strip.setPixelColor(i, strip.Color(255, 0, 0));   // Red
      else strip.setPixelColor(i, strip.Color(0, 0, 255));             // Blue
    }
  }
  strip.show(); 
}

// --- GAUGE DRAWING ---
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

// --- SCREEN 1: RACE ---
void drawScreen1() {
  char textBuffer[32]; 
  u8g2.setFont(u8g2_font_logisoso92_tn); 
  snprintf(textBuffer, sizeof(textBuffer), "%d", currentGear);
  u8g2.drawStr(95, 115, textBuffer);
  
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);
  u8g2.setFont(u8g2_font_profont22_tr);
  u8g2.drawStr(2, 14, "Boost");
  u8g2.drawStr(105, 14, "Gear");
  u8g2.drawStr(190, 14, "SoC");
  u8g2.drawStr(161, 94, "Hy.T"); 

  u8g2.setFont(u8g2_font_profont29_tr);
  snprintf(textBuffer, sizeof(textBuffer), "%d%%", stateOfCharge); 
  u8g2.drawStr(176, 38, textBuffer);

  u8g2.setFont(u8g2_font_profont22_tr);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", boostPressure);
  u8g2.drawStr(20, 65, textBuffer);

  u8g2.setFont(u8g2_font_profont22_tf);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f°C", hybridTemp);
  u8g2.drawUTF8(161, 116, textBuffer); 

  drawGauge(36, 59, 35, 10, 0.0, 2.5, boostPressure);
}

// --- SCREEN 2: WARM-UP ---
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
  u8g2.drawStr(1, 99, "Boost");
  u8g2.drawStr(62, 99, "HybridT");
  u8g2.drawStr(123, 99, "HybridV");
  u8g2.drawStr(195, 99, "Gear");

  u8g2.setFont(u8g2_font_profont22_tr);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", boostPressure); u8g2.drawStr(4, 120, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", hybridTemp); u8g2.drawStr(62, 120, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", hybridVolts); u8g2.drawStr(121, 120, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%d", currentGear); u8g2.drawStr(205, 120, textBuffer);
}

// --- SETUP ---
void setup() {
  Serial.begin(115200); 
  delay(1000); 

  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH); 

  // --- NEOPIXEL INIT & BOOT TEST ---
  strip.begin();
  strip.setBrightness(150); 
  strip.clear();
  
  // Flash RED at boot to confirm hardware connection
  for(int i=0; i<NUM_LEDS; i++) strip.setPixelColor(i, strip.Color(255, 0, 0));
  strip.show();
  delay(1000);
  strip.clear();
  strip.show();

  u8g2.begin();
  u8g2.setContrast(150); 
  
  twai_general_config_t g_config = TW