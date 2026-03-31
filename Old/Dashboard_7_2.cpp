/*
 * ESP32-S3-N16R8 Dual Display CAN Test
 * Hardware SPI & Event-Driven CAN Communication
 */

#include <Arduino.h>
#include <U8g2lib.h>
#include <math.h>
#include <SPI.h>
#include "driver/twai.h"

// =========================================================================
//  NODE IDENTIFICATION TOGGLE
// =========================================================================
#define NODE_ID 1

// --- PIN CONFIG ---
#define SPI_SCK   12
#define SPI_MOSI  11
#define SPI_CS    10
#define SPI_DC    9
#define SPI_RESET 8
#define BACKLIGHT_PIN 4

#define CAN_TX_PIN 47
#define CAN_RX_PIN 48

U8G2_UC1611_EA_DOGXL240_F_4W_HW_SPI u8g2(U8G2_R2, SPI_CS, SPI_DC, SPI_RESET);

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

// =========================================================================
// GAUGE & SCREEN DRAWING 
// =========================================================================
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
  u8g2.drawStr(95, 115, textBuffer);
  
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);
  u8g2.setFont(u8g2_font_profont22_tr);
  u8g2.drawStr(9, 14, "Boost");
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
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", oilTemp);
  u8g2.drawStr(5, 35, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", oilPress);
  u8g2.drawStr(65, 35, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", engineWaterTemp);
  u8g2.drawStr(125, 35, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", icWaterTemp);
  u8g2.drawStr(187, 35, textBuffer);

  u8g2.setFont(u8g2_font_t0_16b_tr);
  u8g2.drawStr(4, 56, "Lambda");
  u8g2.drawStr(61, 56, "IntakeT");
  u8g2.drawStr(135, 56, "EGT");
  u8g2.drawStr(183, 56, "Battery");

  u8g2.setFont(u8g2_font_profont22_tr);
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", lambdaVal);
  u8g2.drawStr(4, 77, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", intakeTemp);
  u8g2.drawStr(62, 77, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", egt);
  u8g2.drawStr(121, 77, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", batteryVolts);
  u8g2.drawStr(182, 77, textBuffer);

  u8g2.setFont(u8g2_font_t0_16b_tr);
  u8g2.drawStr(8, 99, "Boost");
  u8g2.drawStr(62, 99, "HybridT");
  u8g2.drawStr(123, 99, "HybridV");
  u8g2.drawStr(195, 99, "Gear");

  u8g2.setFont(u8g2_font_profont22_tr);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", boostPressure);
  u8g2.drawStr(4, 120, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", hybridTemp);
  u8g2.drawStr(62, 120, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", hybridVolts);
  u8g2.drawStr(121, 120, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%d", currentGear);
  u8g2.drawStr(205, 120, textBuffer);
}

// =========================================================================
// CAN TRANSMIT HELPERS
// =========================================================================
void broadcastData(uint32_t id, uint8_t* payload, uint8_t length) {
  twai_message_t message;
  message.flags = 0;                  
  message.identifier = id;
  message.extd = 0;                   
  message.rtr = 0;
  message.data_length_code = length;
  memcpy(message.data, payload, length);
  twai_transmit(&message, pdMS_TO_TICKS(5));
}

void broadcastAllState(uint32_t my_tx_base) {
  uint8_t payload[8];

  memcpy(&payload[0], &oilTemp, 4); memcpy(&payload[4], &oilPress, 4);
  broadcastData(my_tx_base + 0, payload, 8);
  memcpy(&payload[0], &engineWaterTemp, 4); memcpy(&payload[4], &icWaterTemp, 4);
  broadcastData(my_tx_base + 1, payload, 8);

  memcpy(&payload[0], &lambdaVal, 4); memcpy(&payload[4], &intakeTemp, 4);
  broadcastData(my_tx_base + 2, payload, 8);

  memcpy(&payload[0], &egt, 4); memcpy(&payload[4], &batteryVolts, 4);
  broadcastData(my_tx_base + 3, payload, 8);

  memcpy(&payload[0], &boostPressure, 4);
  memcpy(&payload[4], &hybridTemp, 4);
  broadcastData(my_tx_base + 4, payload, 8);

  memcpy(&payload[0], &hybridVolts, 4);
  payload[4] = (uint8_t)currentGear; payload[5] = (uint8_t)stateOfCharge;
  broadcastData(my_tx_base + 5, payload, 6);
}

// =========================================================================
// SETUP
// =========================================================================
void setup() {
  Serial.begin(115200);
  delay(1000); 

  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);
  
  // 1. FIX: Use -1 for the CS pin so U8g2 can manage it!
  SPI.begin(SPI_SCK, -1, SPI_MOSI, -1); 

  // 2. Set the SPI Clock Speed BEFORE u8g2.begin() for stable initialization
  u8g2.setBusClock(4000000); 

  // 3. Initialize the display
  u8g2.begin();
  
  u8g2.setContrast(150); 
  
  Serial.print("System Ready! Running as NODE ");
  Serial.println(NODE_ID);
  
  // Initialize TWAI (CAN)
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_NORMAL);
  g_config.tx_queue_len = 10; 
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    twai_start();
  }
}

// =========================================================================
// MAIN LOOP
// =========================================================================
void loop() {
  
  uint32_t my_tx_base = (NODE_ID == 1) ? 0x100 : 0x200;
  uint32_t listen_rx_base = (NODE_ID == 1) ? 0x200 : 0x100;
  
  // --- 1. PROCESS INCOMING CAN MESSAGES ---
  twai_message_t rx_msg;
  while (twai_receive(&rx_msg, 0) == ESP_OK) {
    if (rx_msg.identifier >= listen_rx_base && rx_msg.identifier <= listen_rx_base + 5) {
      uint32_t offset = rx_msg.identifier - listen_rx_base;
      switch (offset) {
        case 0: memcpy(&oilTemp, &rx_msg.data[0], 4); memcpy(&oilPress, &rx_msg.data[4], 4); break;
        case 1: memcpy(&engineWaterTemp, &rx_msg.data[0], 4); memcpy(&icWaterTemp, &rx_msg.data[4], 4); break;
        case 2: memcpy(&lambdaVal, &rx_msg.data[0], 4); memcpy(&intakeTemp, &rx_msg.data[4], 4); break;
        case 3: memcpy(&egt, &rx_msg.data[0], 4); memcpy(&batteryVolts, &rx_msg.data[4], 4); break;
        case 4: memcpy(&boostPressure, &rx_msg.data[0], 4); memcpy(&hybridTemp, &rx_msg.data[4], 4); break;
        case 5: memcpy(&hybridVolts, &rx_msg.data[0], 4); currentGear = rx_msg.data[4]; stateOfCharge = rx_msg.data[5]; break;
      }
    }
  }

  // --- 2. SERIAL INPUT (Manual Override & Event Trigger) ---
  bool localChangeMade = false;
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.startsWith("C")) {
      int desiredScreen = input.substring(1).toInt();
      if (desiredScreen == 1 || desiredScreen == 2) activeScreen = desiredScreen;
    }
    else {
      if (input.startsWith("OT")) oilTemp = input.substring(2).toFloat();
      else if (input.startsWith("OP")) oilPress = input.substring(2).toFloat();
      else if (input.startsWith("EWT")) engineWaterTemp = input.substring(3).toFloat();
      else if (input.startsWith("IWT")) icWaterTemp = input.substring(3).toFloat();
      else if (input.startsWith("L")) lambdaVal = input.substring(1).toFloat();
      else if (input.startsWith("IT")) intakeTemp = input.substring(2).toFloat();
      else if (input.startsWith("EGT")) egt = input.substring(3).toFloat();
      else if (input.startsWith("BV")) batteryVolts = input.substring(2).toFloat();
      else if (input.startsWith("BP")) boostPressure = input.substring(2).toFloat();
      else if (input.startsWith("HT")) hybridTemp = input.substring(2).toFloat();
      else if (input.startsWith("HV")) hybridVolts = input.substring(2).toFloat();
      else if (input.startsWith("G")) currentGear = input.substring(1).toInt();
      else if (input.startsWith("SoC")) stateOfCharge = input.substring(3).toInt();
      localChangeMade = true;
    }
  }

  // --- EVENT-DRIVEN CAN BROADCAST ---
  if (localChangeMade) {
    broadcastAllState(my_tx_base);
  }

  // --- DRAW THE SCREEN ---
  u8g2.clearBuffer();          
  if (activeScreen == 1) drawScreen1();
  else if (activeScreen == 2) drawScreen2();
  u8g2.sendBuffer();          
  
  delay(10); 
}