/*
 * ESP32-S3-N16R8 with EA DOGXL240-7 Display
 * Troubleshooting Test: Software SPI & Contrast Sweep
 */

#include <Arduino.h>
#include <U8g2lib.h>

// Your Custom PCB SPI Pins
#define SPI_SCK   12  // CLK
#define SPI_MOSI  11  // SI
#define SPI_CS    10  // CS0
#define SPI_DC    9   // CD / A0
#define SPI_RESET 8   // RESET

// Optional: If you find out you HAVE a backlight pin, define it here:
// #define BACKLIGHT_PIN 13 // (Example pin)

// CHANGED: We are now using 4W_SW_SPI (Software SPI). 
// This forces U8g2 to use your exact Clock and Data pins!
U8G2_UC1611_EA_DOGXL240_F_4W_SW_SPI u8g2(
  U8G2_R0,    // No rotation
  SPI_SCK,    // Explicitly pass Clock
  SPI_MOSI,   // Explicitly pass Data
  SPI_CS,     
  SPI_DC,     
  SPI_RESET   
);

int contrastValue = 0;

void setup() {
  Serial.begin(115200);
  delay(1000); 
  Serial.println("Starting Software SPI & Contrast Test...");

  // If you have a backlight pin, uncomment these lines:
  // pinMode(BACKLIGHT_PIN, OUTPUT);
  // digitalWrite(BACKLIGHT_PIN, HIGH); // Or LOW, depending on your circuit

  // Start the display
  u8g2.begin();
}

void loop() {
  // Clear the internal memory buffer
  u8g2.clearBuffer();          

  // Set font and draw test elements
  u8g2.setFont(u8g2_font_ncenB14_tr); 
  u8g2.drawStr(20, 60, "Testing SPI...");
  
  // Print the current contrast value to the screen
  u8g2.setCursor(20, 100);
  u8g2.print("Contrast: ");
  u8g2.print(contrastValue);

  u8g2.drawFrame(0, 0, 240, 160);

  // Send the drawn buffer to the display
  u8g2.sendBuffer();          

  // Update contrast on the display
  u8g2.setContrast(contrastValue);

  // Print to the serial monitor so you can track it on your PC
  Serial.print("Current Contrast: ");
  Serial.println(contrastValue);

  // Increase contrast for the next loop
  contrastValue += 10;
  if (contrastValue > 200) {
    contrastValue = 50; // Reset back to 0 once it hits the max
  }

  // Wait half a second before changing contrast again
  delay(500); 
}