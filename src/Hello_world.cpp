/*
 * ESP32-S3-N16R8 with EA DOGXL240-7 Display
 * Clean "Hello World" Baseline (Software SPI)
 */

#include <Arduino.h>
#include <U8g2lib.h>

// Custom PCB SPI Pins
#define SPI_SCK   12  // CLK
#define SPI_MOSI  11  // SI
#define SPI_CS    10  // CS0
#define SPI_DC    9   // CD / A0
#define SPI_RESET 8   // RESET
#define BACKLIGHT_PIN 4

U8G2_UC1611_EA_DOGXL240_F_4W_SW_SPI u8g2(
  U8G2_R2,    
  SPI_SCK,    
  SPI_MOSI,   
  SPI_CS,     
  SPI_DC,     
  SPI_RESET   
);

void setup() {
  Serial.begin(115200);
  delay(1000); 
  Serial.println("Starting Hello World...");
  u8g2.begin();
  u8g2.setContrast(150);
  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);  
}

void loop() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.drawStr(40, 80, "Hello World!");
  u8g2.drawFrame(0, 0, 240, 128)
  u8g2.sendBuffer();          
  delay(1000); 
}