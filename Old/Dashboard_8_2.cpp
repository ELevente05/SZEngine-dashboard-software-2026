/*
 * ESP32-S3-N16R8 Hub Transmitter
 * Receives 20Hz Serial Telemetry from Python CSV Playback
 * Encodes and Broadcasts via CAN bus to Dashboard_8
 */

#include <Arduino.h>
#include "driver/twai.h"

// --- PIN CONFIG ---
#define CAN_TX_PIN 47
#define CAN_RX_PIN 48

// --- DYNAMIC DATA VARIABLES ---
float oilTemp = 85.0; 
float oilPress = 4.0;     // Stored in Bar
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
int activeScreen = 1; 
int rpm = 0;

// --- SERIAL & TIMING ---
unsigned long lastBroadcastTime = 0;
char serialBuffer[64];
int serialIndex = 0;

// --- CAN TRANSMIT HELPER ---
void broadcastData(uint32_t id, uint8_t* payload, uint8_t length) {
  twai_message_t message;
  message.flags = 0;
  message.identifier = id;
  message.extd = 0;                   
  message.rtr = 0;
  message.data_length_code = length;
  
  memcpy(message.data, payload, length);
  twai_transmit(&message, pdMS_TO_TICKS(2)); // Short timeout to prevent blocking
}

// --- MAXXECU BIG-ENDIAN ENCODER ---
// Converts a number into the 16-bit Big-Endian format the MaxxECU uses
void encodeBE(uint8_t* data, int offset, int16_t value) {
  data[offset] = (value >> 8) & 0xFF; // High byte
  data[offset + 1] = value & 0xFF;    // Low byte
}

void setup() {
  Serial.begin(115200); 
  delay(1000); 

  Serial.println("HUB NODE READY: Listening for Python Telemetry...");
  
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_NORMAL);
  g_config.tx_queue_len = 20; // 20-message queue to handle 20Hz bursts
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); 
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    twai_start();
  }
}

// --- HIGH SPEED SERIAL PARSER ---
void processCommand(char* cmd) {
  if (cmd[0] == 'C') activeScreen = atoi(&cmd[1]);
  else if (strncmp(cmd, "RPM", 3) == 0) rpm = atoi(&cmd[3]);
  else if (strncmp(cmd, "EWT", 3) == 0) engineWaterTemp = atof(&cmd[3]);
  else if (strncmp(cmd, "IWT", 3) == 0) icWaterTemp = atof(&cmd[3]);
  else if (strncmp(cmd, "EGT", 3) == 0) egt = atof(&cmd[3]);
  else if (strncmp(cmd, "SoC", 3) == 0) stateOfCharge = atoi(&cmd[3]);
  else if (strncmp(cmd, "OT", 2) == 0) oilTemp = atof(&cmd[2]);
  else if (strncmp(cmd, "OP", 2) == 0) oilPress = atof(&cmd[2]);
  else if (strncmp(cmd, "IT", 2) == 0) intakeTemp = atof(&cmd[2]);
  else if (strncmp(cmd, "BV", 2) == 0) batteryVolts = atof(&cmd[2]);
  else if (strncmp(cmd, "BP", 2) == 0) boostPressure = atof(&cmd[2]);
  else if (strncmp(cmd, "HT", 2) == 0) hybridTemp = atof(&cmd[2]);
  else if (strncmp(cmd, "HV", 2) == 0) hybridVolts = atof(&cmd[2]);
  else if (strncmp(cmd, "L", 1) == 0) lambdaVal = atof(&cmd[1]);
  else if (strncmp(cmd, "G", 1) == 0) currentGear = atoi(&cmd[1]);
}

void loop() {
  
  // --- 1. NON-BLOCKING SERIAL INGESTION ---
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      serialBuffer[serialIndex] = '\0';
      processCommand(serialBuffer);
      serialIndex = 0;
    } else if (serialIndex < 63) {
      serialBuffer[serialIndex++] = c;
    }
  }

  // --- 2. FAST CAN BROADCAST (20Hz / Every 50ms) ---
  if (millis() - lastBroadcastTime > 50) {
    lastBroadcastTime = millis();
    uint8_t payload[8];

    // --- MAXXECU FORMAT FRAMES ---

    // Frame 0x520: RPM, Throttle (Empty), Lambda
    memset(payload, 0, 8); 
    encodeBE(payload, 0, rpm);
    encodeBE(payload, 6, (int16_t)(lambdaVal * 1000));
    broadcastData(0x520, payload, 8);

    // Frame 0x530: Battery, IAT, EWT
    memset(payload, 0, 8);
    encodeBE(payload, 0, (int16_t)(batteryVolts * 100));
    encodeBE(payload, 4, (int16_t)(intakeTemp * 10));
    encodeBE(payload, 6, (int16_t)(engineWaterTemp * 10));
    broadcastData(0x530, payload, 8);

    // Frame 0x531: EGT
    memset(payload, 0, 8);
    encodeBE(payload, 6, (int16_t)egt);
    broadcastData(0x531, payload, 8);

    // Frame 0x536: Gear, Oil Pressure, Oil Temp
    memset(payload, 0, 8);
    payload[0] = (uint8_t)currentGear; 
    // Dash expects kPa (* 0.001) to display Bar, so we send Bar * 1000
    encodeBE(payload, 4, (int16_t)(oilPress * 1000)); 
    encodeBE(payload, 6, (int16_t)(oilTemp * 10));
    broadcastData(0x536, payload, 8);


    // --- CUSTOM FLOAT FRAMES ---
    
    // Frame 0x101: Custom IC Water Temp
    memset(payload, 0, 8);
    memcpy(&payload[4], &icWaterTemp, 4);
    broadcastData(0x101, payload, 8);

    // Frame 0x104: Custom Boost Pressure & Hybrid Temp
    memset(payload, 0, 8);
    memcpy(&payload[0], &boostPressure, 4); 
    memcpy(&payload[4], &hybridTemp, 4);
    broadcastData(0x104, payload, 8);

    // Frame 0x105: Custom Hybrid Volts, SoC, Screen State
    memset(payload, 0, 8);
    memcpy(&payload[0], &hybridVolts, 4);
    payload[5] = (uint8_t)stateOfCharge;
    payload[6] = (uint8_t)activeScreen;
    broadcastData(0x105, payload, 7); 
  }
}