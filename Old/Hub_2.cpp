#include <Arduino.h>
#include "driver/twai.h"

// --- PIN CONFIG ---
#define CAN_TX_PIN 47
#define CAN_RX_PIN 48

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
int activeScreen = 1;

unsigned long lastBroadcastTime = 0;

// --- CAN TRANSMIT HELPER ---
void broadcastData(uint32_t id, uint8_t* payload, uint8_t length) {
  twai_message_t message;
  message.flags = 0;
  message.identifier = id;
  message.extd = 0;                   
  message.rtr = 0;
  message.data_length_code = length;
  
  memcpy(message.data, payload, length);
  twai_transmit(&message, pdMS_TO_TICKS(5)); // Give it time to push onto the bus
}

// --- SETUP ---
void setup() {
  Serial.begin(115200); // Updated to 115200
  delay(1000); 

  Serial.println("HUB NODE READY");
  
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_NORMAL);
  g_config.tx_queue_len = 20; // FIX: Increase TX queue so no messages drop locally
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); 
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    twai_start();
  }
}

// --- MAIN LOOP ---
void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim(); 
    
    if (input.startsWith("C")) {
      int desiredScreen = input.substring(1).toInt();
      if (desiredScreen == 1 || desiredScreen == 2) activeScreen = desiredScreen;
    }
    else if (input.startsWith("OT")) oilTemp = input.substring(2).toFloat();
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
  }

  if (millis() - lastBroadcastTime > 100) {
    lastBroadcastTime = millis();
    uint8_t payload[8];

    memcpy(&payload[0], &oilTemp, 4); memcpy(&payload[4], &oilPress, 4);
    broadcastData(0x100, payload, 8);

    memcpy(&payload[0], &engineWaterTemp, 4); memcpy(&payload[4], &icWaterTemp, 4);
    broadcastData(0x101, payload, 8);

    memcpy(&payload[0], &lambdaVal, 4); memcpy(&payload[4], &intakeTemp, 4);
    broadcastData(0x102, payload, 8);

    memcpy(&payload[0], &egt, 4); memcpy(&payload[4], &batteryVolts, 4);
    broadcastData(0x103, payload, 8);

    memcpy(&payload[0], &boostPressure, 4); memcpy(&payload[4], &hybridTemp, 4);
    broadcastData(0x104, payload, 8);

    memcpy(&payload[0], &hybridVolts, 4);
    payload[4] = (uint8_t)currentGear;
    payload[5] = (uint8_t)stateOfCharge;
    payload[6] = (uint8_t)activeScreen;
    broadcastData(0x105, payload, 7); // Payload length is 7 to include screen state
  }
}