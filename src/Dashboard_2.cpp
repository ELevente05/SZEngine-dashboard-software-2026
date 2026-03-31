/*
 * ESP32-S3-N16R8 with EA DOGXL240-7 Display
 * Dynamic Data & CAN Bus Preparation
 */

#include <Arduino.h>
#include <U8g2lib.h>

// Your Custom PCB SPI Pins
#define SPI_SCK   12
#define SPI_MOSI  11
#define SPI_CS    10
#define SPI_DC    9
#define SPI_RESET 8

U8G2_UC1611_EA_DOGXL240_F_4W_SW_SPI u8g2(U8G2_R2, SPI_SCK, SPI_MOSI, SPI_CS, SPI_DC, SPI_RESET);

// ===========================
// 1. DYNAMIC DATA VARIABLES
// ===========================
int currentGear = 8;
float boostPressure = 9.9;
float hyTemp = 99.99;
int stateOfCharge = 100;

void setup() {
  Serial.begin(115200);
  delay(1000); 
  u8g2.begin();
  u8g2.setContrast(150); 
}

void loop() {
  u8g2.clearBuffer();          

  char textBuffer[32]; 

  // --- GEAR ---
  u8g2.setFont(u8g2_font_logisoso92_tn); 
  snprintf(textBuffer, sizeof(textBuffer), "%d", currentGear);
  u8g2.drawStr(95, 115, textBuffer);
  
  // Static labels
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);
  u8g2.setFont(u8g2_font_profont22_tr);
  u8g2.drawStr(2, 14, "Boost P");
  u8g2.drawStr(105, 14, "Gear");
  u8g2.drawStr(190, 14, "SoC");
  u8g2.drawStr(161, 94, "Hy.T"); // Removed [C] to use degree symbol below

  // --- STATE OF CHARGE ---
  u8g2.setFont(u8g2_font_profont29_tr);
  snprintf(textBuffer, sizeof(textBuffer), "%d%%", stateOfCharge);
  u8g2.drawStr(176, 38, textBuffer);

  // --- BOOST PRESSURE ---
  u8g2.setFont(u8g2_font_profont22_tr);
  // %.1f means "float with 1 decimal place"
  snprintf(textBuffer, sizeof(textBuffer), "%.1f Bar", boostPressure);
  u8g2.drawStr(0, 117, textBuffer);

  // --- TEMPERATURE ---
  // CHANGED: Using the _tf version of ProFont22 which includes the degree symbol!
  u8g2.setFont(u8g2_font_profont22_tf);
  snprintf(textBuffer, sizeof(textBuffer), "%.2f°C", hyTemp);
  // CHANGED: drawUTF8 understands the ° character in the string
  u8g2.drawUTF8(161, 116, textBuffer); 

  // --- GRAPHICS ---
  u8g2.drawEllipse(36, 59, 35, 35);
  u8g2.drawLine(36, 59, 15, 80);

  // Send the completely drawn frame to the display exactly once
  u8g2.sendBuffer();          

  // Small delay. Once CAN bus is running, this delay will be determined 
  // by how often you receive CAN messages!
  delay(50); 
}