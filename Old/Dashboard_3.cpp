/*
 * ESP32-S3-N16R8 with EA DOGXL240-7 Display
 * Dynamic Serial Testing & Thick Arc Gauge
 */

#include <Arduino.h>
#include <U8g2lib.h>
#include <math.h>

// PIN Config
#define SPI_SCK   12
#define SPI_MOSI  11
#define SPI_CS    10
#define SPI_DC    9
#define SPI_RESET 8
#define BACKLIGHT_PIN 4

U8G2_UC1611_EA_DOGXL240_F_4W_SW_SPI u8g2(U8G2_R2, SPI_SCK, SPI_MOSI, SPI_CS, SPI_DC, SPI_RESET);

// =========================================================================
// DYNAMIC DATA VARIABLES 
// =========================================================================
int currentGear = 0;
float boostPressure = 0.0;
float hyTemp = 25.0;
int stateOfCharge = 80;

// =========================================================================
// CUSTOM GAUGE DRAWING FUNCTION
// =========================================================================
void drawGauge(int cx, int cy, int radius, int thickness, float minVal, float maxVal, float val) {

  if (val < minVal) val = minVal;
  if (val > maxVal) val = maxVal;

  // Draw the background track
  u8g2.drawCircle(cx, cy, radius);
  u8g2.drawCircle(cx, cy, radius - thickness);

  // Map the value to an angle. 
  // 135 degrees (2.356 rad) is bottom-left. 405 degrees (7.068 rad) is bottom-right.
  float start_angle = 2.356; 
  float end_angle = 7.068;   
  
  float target_angle = start_angle + ((val - minVal) / (maxVal - minVal)) * (end_angle - start_angle);

  // Draw the filled arc using overlapping filled circles
  for (float a = start_angle; a <= target_angle; a += 0.05) {
    int x = cx + (radius - thickness/2) * cos(a);
    int y = cy + (radius - thickness/2) * sin(a);
    u8g2.drawDisc(x, y, thickness/2);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000); 

  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH); 

  u8g2.begin();
  u8g2.setContrast(150); 
  
  Serial.println("System Ready! Type commands to update display:");
  Serial.println("Examples: B1.5 (Boost), G3 (Gear), T85.5 (Temp), S95 (SoC)");
}

void loop() {
  // ---------------------------------------------------------
  // 1. CHECK FOR SERIAL INPUT
  // ---------------------------------------------------------
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.startsWith("B")) boostPressure = input.substring(1).toFloat();
    else if (input.startsWith("G")) currentGear = input.substring(1).toInt();
    else if (input.startsWith("T")) hyTemp = input.substring(1).toFloat();
    else if (input.startsWith("S")) stateOfCharge = input.substring(1).toInt();
    
    Serial.print("Updated -> Boost: "); Serial.print(boostPressure);
    Serial.print(" | Gear: "); Serial.print(currentGear);
    Serial.print(" | Temp: "); Serial.print(hyTemp);
    Serial.print(" | SoC: "); Serial.println(stateOfCharge);
  }

  // ---------------------------------------------------------
  // 2. DRAW THE SCREEN
  // ---------------------------------------------------------
  u8g2.clearBuffer();          
  char textBuffer[32]; 

  // --- GEAR ---
  u8g2.setFont(u8g2_font_logisoso92_tn); 
  snprintf(textBuffer, sizeof(textBuffer), "%d", currentGear);
  u8g2.drawStr(95, 115, textBuffer);
  
  // --- STATIC LABELS ---
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);
  u8g2.setFont(u8g2_font_profont22_tr);
  u8g2.drawStr(2, 14, "Boost P");
  u8g2.drawStr(105, 14, "Gear");
  u8g2.drawStr(190, 14, "SoC");
  u8g2.drawStr(161, 94, "Hy.T"); 

  // --- STATE OF CHARGE ---
  u8g2.setFont(u8g2_font_profont29_tr);
  snprintf(textBuffer, sizeof(textBuffer), "%d%%", stateOfCharge); 
  u8g2.drawStr(176, 38, textBuffer);

  // --- BOOST PRESSURE ---
  u8g2.setFont(u8g2_font_profont22_tr);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f Bar", boostPressure);
  u8g2.drawStr(0, 117, textBuffer);

  // --- TEMPERATURE ---
  u8g2.setFont(u8g2_font_profont22_tf);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f°C", hyTemp);
  u8g2.drawUTF8(161, 116, textBuffer); 

  // --- THE ARC GAUGE ---
  // drawGauge(Center X, Center Y, Radius, Thickness, Min Value, Max Value, Current Value)
  // Default: Maps 0.0 to 2.5 bar into a 270-degree arc
  drawGauge(36, 59, 35, 10, 0.0, 2.5, boostPressure);

  u8g2.sendBuffer();          

  delay(20); 
}