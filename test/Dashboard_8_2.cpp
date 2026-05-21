/*
 * ESP32-S3-N16R8 Hub Transmitter
 * Receives 20Hz Serial Telemetry from Python CSV Playback
 * Encodes and Broadcasts via CAN bus to Dashboard_8
 */

#include <Arduino.h>
#include <stdio.h>
#include "driver/twai.h"

// --- PIN CONFIG ---
#define CAN_TX_PIN 47
#define CAN_RX_PIN 48

// --- VARIABLES ---
int activeScreen = 1; 

// Screen 1 & 2
float oilTemp = 97.0;
float oilPress = 2.4;
float engineWaterTemp = 89.8;
float icWaterTemp = 69.0;
float lambdaVal = 0.98;
float intakeTemp = 43.8;
float egt = 490.0;
float batteryVolts = 13.3;
float boostPressure = 0.3;
float hybridTemp = 26.34;
float hybridVolts = 39.6;
int currentGear = 0;
int stateOfCharge = 400;
int rpm = 4000;

// Screens 3 & 4
float T[16] = {0.0};  // T_1 to T_16
float V[10] = {0.0};  // V_1 to V_10
float V_out = 41.9;
float I_out = 0.0;

// --- SERIAL & TIMING ---
unsigned long lastBroadcastTime = 0;

constexpr uint32_t CAN_ID_MAIN_STATUS_1 = 0x520;
constexpr uint32_t CAN_ID_MAIN_STATUS_2 = 0x521;
constexpr uint32_t CAN_ID_MAIN_STATUS_3 = 0x522;
constexpr uint32_t CAN_ID_MAIN_STATUS_4 = 0x523;
constexpr uint32_t CAN_ID_ACTIVE_SCREEN = 0x524;
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

  Serial.println("HUB NODE READY");
  
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_NORMAL);
  g_config.tx_queue_len = 20; // 20-message queue to handle 20Hz bursts
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); 
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    twai_start();
  }
}

// --- SERIAL PARSER ---
void processCommand(char* cmd) {
  int idx;
  float fval;

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
  else if (strncmp(cmd, "L", 1) == 0 && !isalpha(cmd[1])) lambdaVal = atof(&cmd[1]);
  else if (strncmp(cmd, "G", 1) == 0) currentGear = atoi(&cmd[1]);
  else if (strncmp(cmd, "VOUT=", 5) == 0) V_out = atof(&cmd[5]);
  else if (strncmp(cmd, "IOUT=", 5) == 0) I_out = atof(&cmd[5]);
  else if (sscanf(cmd, "T%d=%f", &idx, &fval) == 2) {
    if (idx >= 1 && idx <= 12) T[idx - 1] = fval;
  } 
  else if (sscanf(cmd, "V%d=%f", &idx, &fval) == 2) {
    if (idx >= 1 && idx <= 10) V[idx - 1] = fval;
  }
}

void loop() {
  
  // --- 1. SERIAL INGESTION ---
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

    // Frame 0x520: RPM, Lambda, Boost, Gear, SoC
    memset(payload, 0, 8);
    encodeBE(payload, 0, rpm);
    encodeBE(payload, 2, (int16_t)(lambdaVal * 1000));
    encodeBE(payload, 4, (int16_t)(boostPressure * 1000));
    payload[6] = (uint8_t)currentGear;
    payload[7] = (uint8_t)stateOfCharge;
    broadcastData(CAN_ID_MAIN_STATUS_1, payload, 8);

    // Frame 0x521: Battery, IAT, EWT, IC water
    memset(payload, 0, 8);
    encodeBE(payload, 0, (int16_t)(batteryVolts * 100));
    encodeBE(payload, 2, (int16_t)(intakeTemp * 10));
    encodeBE(payload, 4, (int16_t)(engineWaterTemp * 10));
    encodeBE(payload, 6, (int16_t)(icWaterTemp * 10));
    broadcastData(CAN_ID_MAIN_STATUS_2, payload, 8);

    // Frame 0x522: Oil pressure, oil temp, EGT, hybrid temp
    memset(payload, 0, 8);
    encodeBE(payload, 0, (int16_t)(oilPress * 1000));
    encodeBE(payload, 2, (int16_t)(oilTemp * 10));
    encodeBE(payload, 4, (int16_t)egt);
    encodeBE(payload, 6, (int16_t)(hybridTemp * 100));
    broadcastData(CAN_ID_MAIN_STATUS_3, payload, 8);

    // Frame 0x523: Hybrid volts
    memset(payload, 0, 8);
    encodeBE(payload, 0, (int16_t)(hybridVolts * 100));
    broadcastData(CAN_ID_MAIN_STATUS_4, payload, 8);

    // Frame 0x524: Active screen only
    memset(payload, 0, 8);
    payload[0] = (uint8_t)activeScreen;
    broadcastData(CAN_ID_ACTIVE_SCREEN, payload, 1);
    
    // Frame 0x600: T_1 to T_4
    memset(payload, 0, 8);
    encodeBE(payload, 0, (int16_t)(T[0] * 10));
    encodeBE(payload, 2, (int16_t)(T[1] * 10));
    encodeBE(payload, 4, (int16_t)(T[2] * 10));
    encodeBE(payload, 6, (int16_t)(T[3] * 10));
    broadcastData(0x600, payload, 8);

    // Frame 0x601: T_5 to T_8
    memset(payload, 0, 8);
    encodeBE(payload, 0, (int16_t)(T[4] * 10));
    encodeBE(payload, 2, (int16_t)(T[5] * 10));
    encodeBE(payload, 4, (int16_t)(T[6] * 10));
    encodeBE(payload, 6, (int16_t)(T[7] * 10));
    broadcastData(0x601, payload, 8);

    // Frame 0x602: T_9 to T_12
    memset(payload, 0, 8);
    encodeBE(payload, 0, (int16_t)(T[8] * 10));
    encodeBE(payload, 2, (int16_t)(T[9] * 10));
    encodeBE(payload, 4, (int16_t)(T[10] * 10));
    encodeBE(payload, 6, (int16_t)(T[11] * 10));
    broadcastData(0x602, payload, 8);

    // Frame 0x610: V_1 to V_4
    memset(payload, 0, 8);
    encodeBE(payload, 0, (int16_t)(V[0] * 10));
    encodeBE(payload, 2, (int16_t)(V[1] * 10));
    encodeBE(payload, 4, (int16_t)(V[2] * 10));
    encodeBE(payload, 6, (int16_t)(V[3] * 10));
    broadcastData(0x610, payload, 8);

    // Frame 0x611: V_5 to V_8
    memset(payload, 0, 8);
    encodeBE(payload, 0, (int16_t)(V[4] * 10));
    encodeBE(payload, 2, (int16_t)(V[5] * 10));
    encodeBE(payload, 4, (int16_t)(V[6] * 10));
    encodeBE(payload, 6, (int16_t)(V[7] * 10));
    broadcastData(0x611, payload, 8);

    // Frame 0x612: V_9, V_10, V_out, I_out
    memset(payload, 0, 8);
    encodeBE(payload, 0, (int16_t)(V[8] * 10));
    encodeBE(payload, 2, (int16_t)(V[9] * 10));
    encodeBE(payload, 4, (int16_t)(V_out * 10));
    encodeBE(payload, 6, (int16_t)(I_out * 10));
    broadcastData(0x612, payload, 8);
  }
}