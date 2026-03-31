/*
 * ESP32-S3 TWAI (CAN Bus) Dual-Node Tester
 * Includes Timestamps & Self-Reception Fix
 */

#include <Arduino.h>
#include "driver/twai.h"

// =========================================================================
// 1. HARDWARE CONFIGURATION TOGGLE
// Set this to 1 when uploading to the DISPLAY.
// Set this to 0 when uploading to the HUB.
// =========================================================================
#define IS_DISPLAY_NODE 0

#if IS_DISPLAY_NODE
  #define CAN_TX_PIN 47 // SN65 'D' Pin
  #define CAN_RX_PIN 48 // SN65 'R' Pin
  String nodeName = "DISPLAY NODE";
  uint32_t myHeartbeatID = 0x111; // Display sends ID 0x111
#else
  #define CAN_TX_PIN 47 // SN65 'D' Pin
  #define CAN_RX_PIN 48 // SN65 'R' Pin
  String nodeName = "HUB NODE";
  uint32_t myHeartbeatID = 0x222; // Hub sends ID 0x222
#endif

// Timer for automated sending
unsigned long lastHeartbeatTime = 0;

// =========================================================================
// TRANSMIT FUNCTION
// =========================================================================
void sendCANMessage(uint32_t id, uint8_t data1, uint8_t data2) {
  twai_message_t message;
  
  // Wipe the structure clean so no hidden flags are randomly triggered
  message.flags = 0; 

  message.identifier = id;            
  message.extd = 0;                   // Standard 11-bit Frame
  message.rtr = 0;                    // Data Frame
  message.data_length_code = 2;       // Sending 2 bytes of data
  message.data[0] = data1;
  message.data[1] = data2;

  if (twai_transmit(&message, pdMS_TO_TICKS(100)) == ESP_OK) {
    // Grab the current time and format it
    float timestamp = millis() / 1000.0;
    
    Serial.print("[");
    Serial.print(timestamp, 3); // Print with 3 decimal places (milliseconds)
    Serial.print("s] -> SENT: ID 0x");
    Serial.print(id, HEX);
    Serial.print(" | Data: ");
    Serial.print(data1, HEX);
    Serial.print(" ");
    Serial.println(data2, HEX);
  } else {
    Serial.println("-> FAILED to send message! Check wiring/resistors.");
  }
}

// =========================================================================
// SETUP
// =========================================================================
void setup() {
  Serial.begin(115200);
  delay(2000); 
  
  Serial.println("=====================================");
  Serial.print("Starting CAN Test as: ");
  Serial.println(nodeName);
  Serial.print("TX Pin: "); Serial.print(CAN_TX_PIN);
  Serial.print(" | RX Pin: "); Serial.println(CAN_RX_PIN);
  Serial.println("=====================================");

  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    if (twai_start() == ESP_OK) {
      Serial.println("TWAI Driver started successfully!");
    } else {
      Serial.println("Failed to start TWAI driver!");
    }
  } else {
    Serial.println("Failed to install driver!");
  }
}

// =========================================================================
// MAIN LOOP
// =========================================================================
void loop() {
  // 1. CHECK FOR INCOMING CAN MESSAGES
  twai_message_t rx_msg;
  
  if (twai_receive(&rx_msg, 0) == ESP_OK) {
    // Grab the current time for the received message
    float timestamp = millis() / 1000.0;

    Serial.print("[");
    Serial.print(timestamp, 3); 
    Serial.print("s] <- RECEIVED: ID 0x");
    Serial.print(rx_msg.identifier, HEX);
    Serial.print(" | DLC: ");
    Serial.print(rx_msg.data_length_code);
    Serial.print(" | Data: ");
    for (int i = 0; i < rx_msg.data_length_code; i++) {
      // Print a leading zero for single-digit hex numbers to keep it neat
      if(rx_msg.data[i] < 0x10) Serial.print("0");
      Serial.print(rx_msg.data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }

  // 2. AUTOMATIC HEARTBEAT (Sends every 2 seconds)
  if (millis() - lastHeartbeatTime > 10000) {
    lastHeartbeatTime = millis();
    // Remember to change the dummy data slightly if you want to see it change!
    if (IS_DISPLAY_NODE) {
       sendCANMessage(myHeartbeatID, 0xAA, 0xBB);
    } else {
       sendCANMessage(myHeartbeatID, 0xCC, 0xDD);
    }
  }
}