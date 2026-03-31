/*
 * ESP32-S3-N16R8 with EA DOGXL240-7 Display
 * Custom Lopaka UI Test
 */

#include <Arduino.h>
#include <U8g2lib.h>

// Your Custom PCB SPI Pins
#define SPI_SCK   12
#define SPI_MOSI  11
#define SPI_CS    10
#define SPI_DC    9
#define SPI_RESET 8

// U8g2 Constructor (R2 rotation flips the screen 180 degrees)
U8G2_UC1611_EA_DOGXL240_F_4W_SW_SPI u8g2(
  U8G2_R2,    
  SPI_SCK,    
  SPI_MOSI,   
  SPI_CS,     
  SPI_DC,     
  SPI_RESET   
);

// =========================================================================
// PLACEHOLDER IMAGE ARRAY
// Replace this block with the actual 'image_Layer_5_bits' array from Lopaka!
// This creates a simple 16x24 hollow square so the code compiles.
// =========================================================================
static const unsigned char image_Layer_5_bits[] U8X8_PROGMEM = {
  0xff, 0xff, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 
  0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 
  0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 
  0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0xff, 0xff
};
// =========================================================================

void setup() {
  Serial.begin(115200);
  delay(1000); 

  u8g2.begin();
  
  u8g2.setContrast(150); 
}

void loop() {
  u8g2.clearBuffer();          

  // --- START OF LOPAKA UI CODE ---
  
  u8g2.setFont(u8g2_font_logisoso92_tn); 
  u8g2.drawStr(95, 115, "8");
  
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);
  //u8g2.drawXBM(198, 41, 16, 24, image_Layer_3_bits);

  u8g2.setFont(u8g2_font_profont29_tr);
  u8g2.drawStr(176, 38, "100%");

  u8g2.setFont(u8g2_font_profont22_tr);
  u8g2.drawStr(2, 14, "Boost P");

  u8g2.drawStr(172, 116, "99,99");

  u8g2.drawStr(105, 14, "Gear");

  u8g2.drawEllipse(36, 59, 35, 35);

  u8g2.drawStr(190, 14, "SoC");

  u8g2.drawStr(0, 117, "9.9 Bar");

  u8g2.drawLine(36, 59, 15, 80);

  u8g2.drawStr(161, 94, "Hy.T[C]");

  u8g2.sendBuffer();



  // --- END OF LOPAKA UI CODE ---

  u8g2.sendBuffer();          

  delay(50); 
}