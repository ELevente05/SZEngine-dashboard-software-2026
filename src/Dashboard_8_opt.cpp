/*
 * ESP32-S3-N16R8 Receiver Dashboard
 * Refactored for FreeRTOS Safety, Concurrency, and Automotive Robustness
 * Updated to include per-eFuse 3-Second Pop-Up Warnings
 * BSPD Setup Screen included
 */

#include <Arduino.h>
#include <U8g2lib.h>
#include <math.h>
#include "driver/twai.h"
#include <Adafruit_NeoPixel.h>

// --- PIN CONFIG ---
#define SPI_SCK   12
#define SPI_MOSI  11
#define SPI_CS    10
#define SPI_DC    9
#define SPI_RESET 8
#define BACKLIGHT_PIN 4
#define CAN_TX_PIN 47
#define CAN_RX_PIN 48
#define LED_PIN 7
#define NUM_LEDS 9

U8G2_UC1611_EA_DOGXL240_F_4W_SW_SPI u8g2(U8G2_R2, SPI_SCK, SPI_MOSI, SPI_CS, SPI_DC, SPI_RESET);
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// --- BOOT LOGOS ---
static const unsigned char PROGMEM SZEngine_logo[984] = {
  0x00,0x80,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x01,0x00,
  0x00,0xE0,0xFC,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x03,0x00,
  0x00,0xF0,0xEF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x03,0x00,
  0x00,0xF8,0x7F,0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x07,0x00,
  0x00,0xF8,0xFF,0xF3,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x07,0x00,
  0x00,0xFC,0xFF,0x9F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x0F,0x00,
  0x00,0xFC,0xFF,0xFF,0xFC,0xFF,0xFF,0xFF,0xFF,0xFF,0x0F,0x00,
  0x00,0xFE,0xFF,0xFF,0xE7,0xFF,0xFF,0xFF,0xFF,0xFF,0x1F,0x00,
  0x00,0xFE,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0xFF,0xFF,0x3F,0x00,
  0x00,0xFF,0xFF,0xFF,0xFF,0xF1,0xFF,0xFF,0xFF,0xFF,0x3F,0x00,
  0x00,0xFF,0xFF,0xFF,0xFF,0xC1,0xFF,0xFF,0xFF,0xFF,0x7F,0x00,
  0x80,0xFF,0xFF,0xFF,0xFF,0x00,0xFC,0xFF,0xFF,0xFF,0x7F,0x00,
  0xC0,0xFF,0xFF,0xFF,0xFF,0x00,0xE0,0xFF,0xFF,0xFF,0xFF,0x00,
  0xC0,0xFF,0xFF,0xFF,0x7F,0x00,0x80,0xFF,0xFF,0xFF,0xFF,0x00,
  0xE0,0xFF,0xFF,0xFF,0x7F,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x01,
  0xE0,0xFF,0xFF,0xFF,0x3F,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x03,
  0xF0,0xFF,0xFF,0xFF,0x3F,0x00,0x00,0xFE,0xFF,0xFF,0xFF,0x03,
  0xF8,0xFF,0xFF,0xFF,0x1F,0x00,0x00,0xFE,0xFF,0xFF,0xFF,0x07,
  0xF8,0xFF,0xFF,0xFF,0x0F,0x00,0x00,0xFC,0xFF,0xFF,0xFF,0x07,
  0xFC,0xFF,0xFF,0xFF,0x0F,0x00,0x00,0xFC,0xFF,0xFF,0xFF,0x0F,
  0xFC,0xFF,0xFF,0xFF,0x07,0x00,0x00,0xF8,0xFF,0xFF,0xFF,0x0F,
  0xFE,0xFF,0xFF,0xFF,0x07,0x00,0x00,0xF0,0xFF,0xFF,0xFF,0x1F,
  0xFE,0xFF,0xFF,0xFF,0x03,0x00,0x00,0xF0,0xFF,0xFF,0xFF,0x3F,
  0xFF,0xFF,0xFF,0xFF,0x03,0x00,0x00,0xE0,0xFF,0xFF,0xFF,0x3F,
  0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0xE0,0xFF,0xFF,0xFF,0x1F,
  0xFE,0xFF,0xFF,0xFF,0x00,0x00,0x00,0xC0,0xFF,0xFF,0xFF,0x2F,
  0xFE,0xFF,0xFF,0xFF,0x00,0x00,0x00,0xC0,0xFF,0xFF,0xFF,0x17,
  0xFC,0xFF,0xFF,0x7F,0x00,0x00,0x00,0x80,0xFF,0xFF,0xFF,0x1F,
  0xF8,0xFF,0xFF,0x7F,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0x0F,
  0xF8,0xFF,0xFF,0x3F,0x00,0x00,0x00,0x00,0xFE,0xFF,0x7F,0x07,
  0xF0,0xFF,0xFF,0x3F,0x00,0x00,0x00,0x00,0xFE,0xFF,0xBF,0x07,
  0xF0,0xFF,0xFF,0x1F,0x00,0x00,0x00,0x00,0xFE,0xFF,0xDF,0x03,
  0xE0,0xFF,0xFF,0x0F,0x00,0x00,0x00,0x00,0xFC,0xFF,0xEF,0x03,
  0xE0,0xFF,0xFF,0x0F,0x00,0x00,0x00,0x00,0xF8,0xFF,0xF7,0x01,
  0xC0,0xFF,0xFF,0x07,0x00,0x00,0x00,0x00,0xF8,0xFF,0xFB,0x00,
  0x80,0xFF,0xFF,0x07,0x00,0x00,0x00,0x00,0xF0,0xFF,0xFD,0x00,
  0x80,0xFF,0xFF,0x03,0x00,0x00,0x00,0x00,0xE0,0xFF,0x7E,0x00,
  0x00,0xFF,0xFF,0x03,0x00,0x00,0x00,0x00,0xE0,0xFF,0x7F,0x00,
  0x00,0xFF,0xFF,0x01,0x00,0x00,0x00,0x00,0xE0,0xFF,0x3F,0x00,
  0x00,0xFE,0xFF,0x03,0x00,0x00,0x00,0x00,0xC0,0xEF,0x3F,0x00,
  0x00,0xFE,0xFF,0x03,0x00,0x00,0x00,0x00,0x80,0xF7,0x1F,0x00,
  0x00,0xFC,0xFF,0x03,0x00,0x00,0x00,0x00,0x80,0xFB,0x0F,0x00,
  0x00,0xF8,0xFF,0x03,0x00,0x00,0x00,0x00,0x00,0xFD,0x0F,0x00,
  0x00,0xF8,0xFF,0x03,0x00,0x00,0x00,0x00,0x00,0xFE,0x07,0x00,
  0x00,0xF0,0xFF,0x07,0x00,0x00,0x00,0x00,0x00,0xFF,0x07,0x00,
  0x00,0xE0,0xFF,0x07,0x00,0x00,0x00,0x00,0x80,0xFF,0x03,0x00,
  0x00,0xE0,0xFF,0x07,0x00,0x00,0x00,0x00,0xC0,0xFF,0x03,0x00,
  0x00,0xE0,0xFF,0x07,0x00,0x00,0x00,0x00,0xE0,0xFF,0x01,0x00,
  0x00,0xC0,0xFF,0x07,0x00,0x00,0x00,0x00,0xF0,0xFF,0x00,0x00,
  0x00,0x80,0xFF,0x0F,0x00,0x00,0x00,0x00,0xFC,0xFF,0x00,0x00,
  0x00,0x80,0xFF,0x0F,0x00,0x00,0x00,0x00,0xFE,0x7F,0x00,0x00,
  0x00,0x00,0xFF,0x0F,0x00,0x00,0x00,0x00,0xFF,0x7F,0x00,0x00,
  0x00,0x00,0xFF,0x0F,0x00,0x00,0x00,0x80,0xFF,0x3F,0x00,0x00,
  0x00,0x00,0xFE,0xEF,0xFF,0xFF,0xFF,0xFF,0xFF,0x3F,0x00,0x00,
  0x00,0x00,0xFE,0xEF,0xFF,0xFF,0xFF,0xFF,0xFF,0x1F,0x00,0x00,
  0x00,0x00,0xFC,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x0F,0x00,0x00,
  0x00,0x00,0xF8,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x0F,0x00,0x00,
  0x00,0x00,0xF8,0xDF,0xFF,0xFF,0xFF,0xFF,0xFF,0x07,0x00,0x00,
  0x00,0x00,0xF0,0xDF,0xFF,0xFF,0xFF,0xFF,0xFF,0x07,0x00,0x00,
  0x00,0x00,0xF0,0xDF,0xFF,0xFF,0xFF,0xFF,0xFF,0x03,0x00,0x00,
  0x00,0x00,0xE0,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x03,0x00,0x00,
  0x00,0x00,0xC0,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,
  0x00,0x00,0xC0,0xBF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,
  0x00,0x00,0x80,0xBF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,
  0x00,0x00,0x80,0xBF,0xFF,0xFF,0xFF,0xFF,0x7F,0x00,0x00,0x00,
  0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0x7F,0x00,0x00,0x00,
  0x00,0x00,0x00,0xFE,0xFF,0xFF,0xFF,0xFF,0x3F,0x00,0x00,0x00,
  0x00,0x00,0x00,0xFE,0xFF,0xFF,0xFF,0xFF,0x3F,0x00,0x00,0x00,
  0x00,0x00,0x00,0x7E,0xFF,0xFF,0xFF,0xFF,0x1F,0x00,0x00,0x00,
  0x00,0x00,0x00,0x7C,0xFF,0xFF,0xFF,0xFF,0x0F,0x00,0x00,0x00,
  0x00,0x00,0x00,0x78,0xFF,0xFF,0xFF,0xFF,0x0F,0x00,0x00,0x00,
  0x00,0x00,0x00,0xF8,0xFF,0xFF,0xFF,0xFF,0x07,0x00,0x00,0x00,
  0x00,0x00,0x00,0xF0,0xFF,0xFF,0xFF,0xFF,0x07,0x00,0x00,0x00,
  0x00,0x00,0x00,0xF0,0xFE,0xFF,0xFF,0xFF,0x03,0x00,0x00,0x00,
  0x00,0x00,0x00,0xE0,0xFE,0xFF,0xFF,0xFF,0x03,0x00,0x00,0x00,
  0x00,0x00,0x00,0xC0,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,
  0x00,0x00,0x00,0xC0,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x80,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x80,0xFD,0xFF,0xFF,0x7F,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0xFD,0xFF,0xFF,0x7F,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0xFD,0xFF,0xFF,0x3F,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0xFE,0xFF,0xFF,0x1F,0x00,0x00,0x00,0x00
};

static const unsigned char PROGMEM SZEngine_title[756] = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xE0,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0xC0,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xE7,0xFF,0xFF,0x7F,0xFE,0x01,0x00,0xE0,0xFF,0xFF,0xFF,0xFF,0xF8,0x07,0x00,0x80,0xFF,0xFF,0xFF,0x1F,
  0x00,0xE0,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xF3,0xFF,0xFF,0x3F,0xFF,0x03,0x00,0xF0,0xFF,0xFF,0xFF,0xFF,0xF8,0x0F,0x00,0xC0,0xFF,0xFF,0xFF,0x1F,
  0x00,0xF0,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xF1,0xFF,0xFF,0x1F,0xFF,0x07,0x00,0xF0,0xFF,0xFF,0xFF,0x7F,0xFE,0x0F,0x00,0xE0,0xFF,0xFF,0xFF,0x0F,
  0x00,0xF0,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xF9,0xFF,0xFF,0x9F,0xFF,0x07,0x00,0xF8,0xFF,0xFF,0xFF,0x7F,0xFE,0x1F,0x00,0xE0,0xFF,0xFF,0xFF,0x0F,
  0x00,0xF8,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xF8,0xFF,0xFF,0x8F,0xFF,0x0F,0x00,0xF8,0xFF,0xFF,0xFF,0x1F,0xFF,0x3F,0x00,0xF0,0xFF,0xFF,0xFF,0x07,
  0x00,0xF8,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFC,0xFF,0xFF,0xCF,0xFF,0x0F,0x00,0xFC,0xFF,0xFF,0xFF,0x1F,0xFF,0x3F,0x00,0xF0,0xFF,0xFF,0xFF,0x07,
  0x00,0xFC,0x03,0x00,0x00,0x00,0x80,0x7F,0x00,0x00,0x00,0xE0,0xFF,0x1F,0x00,0xFE,0x01,0x00,0x00,0x80,0xFF,0x7F,0x00,0xF8,0x07,0x00,0xF8,0x03,
  0x00,0xFE,0x01,0x00,0x00,0x00,0x80,0x7F,0x00,0x00,0x00,0xE0,0xDF,0x3F,0x00,0xFE,0x00,0x00,0x00,0x80,0x7F,0xFF,0x00,0xF8,0x07,0x00,0xF8,0x01,
  0x00,0xFE,0xFF,0xFF,0xF3,0xFF,0xFF,0x3F,0xFE,0xFF,0x1F,0xF0,0x8F,0x7F,0x00,0xFF,0xFC,0xFF,0xFF,0xC7,0x3F,0xFF,0x00,0xFC,0xF3,0xFF,0xFF,0x01,
  0x00,0xFF,0xFF,0xFF,0xF3,0xFF,0xFF,0x1F,0xFF,0xFF,0x1F,0xF0,0x8F,0x7F,0x00,0xFF,0xFC,0xFF,0xFF,0xC7,0x1F,0xFE,0x00,0xFE,0xF1,0xFF,0xFF,0x00,
  0x00,0xFF,0xFF,0xFF,0xF1,0xFF,0xFF,0x1F,0xFF,0xFF,0x0F,0xF8,0x07,0x7F,0x80,0x7F,0xFE,0xFF,0xFF,0xE7,0x1F,0xFE,0x01,0xFE,0xF9,0xFF,0xFF,0x00,
  0x80,0xFF,0xFF,0xFF,0xF9,0xFF,0xFF,0x8F,0xFF,0xFF,0x0F,0xF8,0x07,0xFF,0x80,0x3F,0xFE,0xFF,0xFF,0xF3,0x0F,0xFC,0x01,0xFF,0xF8,0xFF,0x7F,0x00,
  0x80,0xFF,0xFF,0xFF,0xFC,0xFF,0xFF,0x8F,0xFF,0xFF,0x07,0xFC,0x01,0xFE,0xC1,0x1F,0xFF,0xFF,0xFF,0xF1,0x0F,0xF8,0x07,0xFF,0xFC,0xFF,0x7F,0x00,
  0xC0,0xFF,0xFF,0xFF,0xFC,0xFF,0xFF,0xC7,0xFF,0xFF,0x07,0xFE,0x01,0xFE,0xE1,0x1F,0xFF,0xFF,0xFF,0xF1,0x07,0xF8,0x87,0x7F,0xFE,0xFF,0x7F,0x00,
  0x00,0x00,0x80,0x7F,0xFE,0x01,0x00,0x00,0x00,0x00,0x00,0xFE,0x01,0xFC,0xE1,0x1F,0x00,0x00,0xFF,0xF8,0x07,0xF0,0x87,0x7F,0x00,0x00,0x00,0x00,
  0x00,0x00,0x80,0x7F,0xFE,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0xF8,0xF3,0x0F,0x00,0x80,0xFF,0xFC,0x03,0xF0,0xCF,0x3F,0x00,0x00,0x00,0x00,
  0x00,0x00,0xE0,0x1F,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x7F,0x00,0xF8,0xF7,0x07,0x00,0x80,0x7F,0xFE,0x01,0xE0,0xFF,0x1F,0x00,0x00,0x00,0x00,
  0x00,0x00,0xE0,0x1F,0x7F,0x00,0x00,0x00,0x00,0x00,0x80,0x7F,0x00,0xF8,0xFF,0x07,0x00,0xC0,0x7F,0xFE,0x01,0xE0,0xFF,0x1F,0x00,0x00,0x00,0x00,
  0xF8,0xFF,0xFF,0x9F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x7F,0x00,0xF0,0xFF,0xE7,0xFF,0xFF,0x3F,0xFE,0x00,0xC0,0xFF,0x8F,0xFF,0xFF,0x07,0x00,
  0xF8,0xFF,0xFF,0x8F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x3F,0x00,0xE0,0xFF,0xF1,0xFF,0xFF,0x1F,0xFF,0x00,0x80,0xFF,0xCF,0xFF,0xFF,0x07,0x00,
  0xFC,0xFF,0xFF,0xE7,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x1F,0x00,0xE0,0xFF,0xF9,0xFF,0xFF,0x8F,0x7F,0x00,0x80,0xFF,0xE7,0xFF,0xFF,0x03,0x00,
  0xFE,0xFF,0xFF,0xE7,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x0F,0x00,0xC0,0xFF,0xF8,0xFF,0xFF,0x8F,0x3F,0x00,0x00,0xFF,0xE3,0xFF,0xFF,0x01,0x00,
  0xFE,0xFF,0xFF,0xE7,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x0F,0x00,0x80,0xFF,0xF8,0xFF,0xFF,0xCF,0x3F,0x00,0x00,0xFF,0xF3,0xFF,0xFF,0x01,0x00,
  0xFF,0xFF,0xFF,0xF3,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x0F,0x00,0x80,0x7F,0xFC,0xFF,0xFF,0xE7,0x1F,0x00,0x00,0xFE,0xF1,0xFF,0xFF,0x00,0x00
};

// --- DATA STRUCTURE (Thread-Safe Representation) ---
struct VehicleData {
  int activeScreen = 1;

  int rpm = 4000;
  int speed = 0;       
  int currentGear = 0; 
  float stateOfCharge = 400.0f;
  float boostPressure = 0.3f;
  
  float oilTemp = 97.0f;
  float oilPress = 53.4f;
  float engineWaterTemp = 89.8f;
  float icWaterTemp = 69.0f;
  float lambdaVal = 0.98f;
  float intakeTemp = 43.8f;
  float egt = 490.0f;
  float batteryVolts = 13.3f;
  float hybridTemp = 26.5f;
  float hybridVolts = 41.85f;

  float T_1 = 26.0f, T_2 = 25.9f, T_3 = 25.9f, T_4 = 25.9f;
  float T_5 = 25.8f, T_6 = 26.2f, T_7 = 25.9f, T_8 = 26.3f;
  float T_9 = 25.7f, T_10 = 26.1f, T_11 = 25.6f, T_12 = 26.4f;
  float T_13 = 25.5f, T_14 = 26.3f, T_15 = 25.8f, T_16 = 26.5f;

  float V_1 = 4.13f, V_2 = 4.14f, V_3 = 4.15f, V_4 = 4.16f;
  float V_5 = 4.17f, V_6 = 4.18f, V_7 = 4.19f, V_8 = 4.13f;
  float V_9 = 4.20f, V_10 = 4.20f, V_all = 41.85f, I_out = 0.0f;

  float tpsPercent = 0.0f;
  float brakePressKpa = 0.0f;
  float tpsVoltage = 0.0f;
  float brakeVoltage = 0.0f;
  float capturedTpsVoltage = 0.0f;
  float capturedBrakeVoltage = 0.0f;
  bool tpsThresholdMet = false;
  bool brakeThresholdMet = false;

  bool hasWarning = false;
  const char* warningMsg = nullptr;
  bool pduVoltError = false;
  bool pduPowerError = false;
  bool pduFetError = false;
  bool efuseFaultActive[8] = {false};
};

// Global State and Mutex to prevent data tearing across CPU Cores
VehicleData globalVehicleState;
SemaphoreHandle_t stateMutex;

// --- SETTINGS & TIMERS ---
constexpr int rpmStart = 6000;
constexpr int rpmMax = 9500;
unsigned long lastScreenUpdate = 0; 

// --- TASK HANDLE ---
TaskHandle_t TaskCAN;

// --- HELPERS ---
constexpr uint32_t CAN_ID_ACTIVE_SCREEN = 0x524;

inline int16_t parseLE(const uint8_t* data, int offset) { 
  uint16_t raw_val = static_cast<uint16_t>(data[offset]) | (static_cast<uint16_t>(data[offset + 1]) << 8);
  return raw_val;
}

// =========================================================================
// --- CORE 0: DEDICATED CAN BUS TASK ---
// =========================================================================
void TaskCANcode(void * pvParameters) {
  for(;;) { // Infinite FreeRTOS Loop
    twai_message_t rx_msg;
    
    if (twai_receive(&rx_msg, pdMS_TO_TICKS(1)) == ESP_OK) {
      // Lock data structure to update safely
      if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        switch (rx_msg.identifier) {

          case 0x520: // RPM, MAP/Boost, Lambda
              globalVehicleState.rpm = parseLE(rx_msg.data, 0); 
              globalVehicleState.boostPressure = (parseLE(rx_msg.data, 4) * 0.001f) - 1;
              globalVehicleState.lambdaVal = parseLE(rx_msg.data, 6); 
            break;
            
          case 0x524: // ACTIVE SCREEN
              globalVehicleState.activeScreen = rx_msg.data[0];
            break;

          case 0x530: // Battery Volts, Intake Temp
              globalVehicleState.batteryVolts = parseLE(rx_msg.data, 0) * 0.01f;
              globalVehicleState.intakeTemp = parseLE(rx_msg.data, 4) * 0.1f;
            break;

          case 0x531: // EGT 1
              globalVehicleState.egt = parseLE(rx_msg.data, 6);
            break;

          case 0x532: // IC Water Temp
              globalVehicleState.icWaterTemp = static_cast<float>(parseLE(rx_msg.data, 4));
            break;
            
          case 0x533: // Engine Water Temp
              globalVehicleState.engineWaterTemp = static_cast<float>(parseLE(rx_msg.data, 0) * 0.1f);
            break;

          case 0x538: // Engine Oil Press, Engine Oil Temp
              globalVehicleState.oilPress = static_cast<float>(parseLE(rx_msg.data, 0)) * 0.1f;
              globalVehicleState.oilTemp = static_cast<float>(parseLE(rx_msg.data, 2)) * 0.1f;
            break;

          case 0x543: // Current Gear
              globalVehicleState.currentGear = rx_msg.data[0]; 
              globalVehicleState.speed = rx_msg.data[1];
            break;

          case 0x600: // Temps 1-4
            if (rx_msg.data_length_code >= 8) {
              globalVehicleState.T_1 = parseLE(rx_msg.data, 0) * 0.1f;
              globalVehicleState.T_2 = parseLE(rx_msg.data, 2) * 0.1f;
              globalVehicleState.T_3 = parseLE(rx_msg.data, 4) * 0.1f;
              globalVehicleState.T_4 = parseLE(rx_msg.data, 6) * 0.1f;
            }
            break;

          case 0x601: // Temps 5-8
            if (rx_msg.data_length_code >= 8) {
              globalVehicleState.T_5 = parseLE(rx_msg.data, 0) * 0.1f;
              globalVehicleState.T_6 = parseLE(rx_msg.data, 2) * 0.1f;
              globalVehicleState.T_7 = parseLE(rx_msg.data, 4) * 0.1f;
              globalVehicleState.T_8 = parseLE(rx_msg.data, 6) * 0.1f;
            }
            break;

          case 0x602: // Temps 9-12
            if (rx_msg.data_length_code >= 8) {
              globalVehicleState.T_9 = parseLE(rx_msg.data, 0) * 0.1f;
              globalVehicleState.T_10 = parseLE(rx_msg.data, 2) * 0.1f;
              globalVehicleState.T_11 = parseLE(rx_msg.data, 4) * 0.1f;
              globalVehicleState.T_12 = parseLE(rx_msg.data, 6) * 0.1f;
            }
            break;

          case 0x603: // Temps 13-16
            if (rx_msg.data_length_code >= 8) {
              globalVehicleState.T_13 = parseLE(rx_msg.data, 0) * 0.1f;
              globalVehicleState.T_14 = parseLE(rx_msg.data, 2) * 0.1f;
              globalVehicleState.T_15 = parseLE(rx_msg.data, 4) * 0.1f;
              globalVehicleState.T_16 = parseLE(rx_msg.data, 6) * 0.1f;
            }
            break;

          case 0x610: // Volts 1-4
            if (rx_msg.data_length_code >= 8) {
              globalVehicleState.V_1 = parseLE(rx_msg.data, 0) * 0.1f;
              globalVehicleState.V_2 = parseLE(rx_msg.data, 2) * 0.1f;
              globalVehicleState.V_3 = parseLE(rx_msg.data, 4) * 0.1f;
              globalVehicleState.V_4 = parseLE(rx_msg.data, 6) * 0.1f;
            }
            break;

          case 0x611: // Volts 5-8
            if (rx_msg.data_length_code >= 8) {
              globalVehicleState.V_5 = parseLE(rx_msg.data, 0) * 0.1f;
              globalVehicleState.V_6 = parseLE(rx_msg.data, 2) * 0.1f;
              globalVehicleState.V_7 = parseLE(rx_msg.data, 4) * 0.1f;
              globalVehicleState.V_8 = parseLE(rx_msg.data, 6) * 0.1f;
            }
            break;

          case 0x612: // Volts 9-10 & Output
            if (rx_msg.data_length_code >= 8) {
              globalVehicleState.V_9 = parseLE(rx_msg.data, 0) * 0.1f;
              globalVehicleState.V_10 = parseLE(rx_msg.data, 2) * 0.1f;
              globalVehicleState.V_all = parseLE(rx_msg.data, 4) * 0.1f;
              globalVehicleState.I_out = parseLE(rx_msg.data, 6) * 0.1f;
            }
            break;

          case 0x620: // PDU Global Error Status
            if (rx_msg.data_length_code >= 1) {
              globalVehicleState.pduVoltError  = (rx_msg.data[0] & 0x01) != 0;
              globalVehicleState.pduPowerError = (rx_msg.data[0] & 0x02) != 0;
              globalVehicleState.pduFetError   = (rx_msg.data[0] & 0x04) != 0;
            }
            break;

          case 0x630: // BSPD Sensor Values (Example ID - Adjust if needed)
            //if (rx_msg.data_length_code >= 8) {
              // Adjust multipliers to match your CAN configuration scaling
              globalVehicleState.tpsPercent = parseLE(rx_msg.data, 0) * 0.1f; 
              globalVehicleState.brakePressKpa = parseLE(rx_msg.data, 2) * 1.0f; // Assuming raw kPa
              globalVehicleState.tpsVoltage = parseLE(rx_msg.data, 4) * 0.01f;  
              globalVehicleState.brakeVoltage = parseLE(rx_msg.data, 6) * 0.01f;

              // --- THRESHOLD CAPTURE LOGIC ---
              
              // 1. TPS Capture (Threshold: 25%)
              if (globalVehicleState.tpsPercent >= 25.0f && !globalVehicleState.tpsThresholdMet) {
                globalVehicleState.capturedTpsVoltage = globalVehicleState.tpsVoltage;
                globalVehicleState.tpsThresholdMet = true; // Latch it
              } else if (globalVehicleState.tpsPercent < 20.0f) {
                globalVehicleState.tpsThresholdMet = false; // Reset the latch
              }

              // 2. Brake Capture (Threshold: 3000 kPa / 30 bar)
              if (globalVehicleState.brakePressKpa >= 3000.0f && !globalVehicleState.brakeThresholdMet) {
                globalVehicleState.capturedBrakeVoltage = globalVehicleState.brakeVoltage;
                globalVehicleState.brakeThresholdMet = true; // Latch it
              } else if (globalVehicleState.brakePressKpa < 2500.0f) {
                globalVehicleState.brakeThresholdMet = false; // Reset the latch
              }
            //}
            break;

          // --- Per-eFuse Fault Data (0x710 to 0x717) ---
          case 0x710: case 0x711: case 0x712: case 0x713:
          case 0x714: case 0x715: case 0x716: case 0x717:
            if (rx_msg.data_length_code >= 8) {
              int channel = rx_msg.identifier - 0x710;
              uint16_t status_word = parseLE(rx_msg.data, 6);
              const uint16_t FAULT_MASK = 0x703F; 
              globalVehicleState.efuseFaultActive[channel] = ((status_word & FAULT_MASK) != 0);
            }
            break;
        }
        
        xSemaphoreGive(stateMutex);
      }
    }
  }
}

// =========================================================================
// --- CORE 1: GRAPHICS & MAIN LOOP ---
// =========================================================================

void updateLEDs(int currentRpm) {
  int numLedsToLight = 0;
  bool redline = false;

  if (currentRpm >= rpmMax) {
    strip.setBrightness(250);
    redline = true;
  } else if (rpmMax > rpmStart && currentRpm >= rpmStart) {
    strip.setBrightness(150); 
    numLedsToLight = static_cast<int>((currentRpm - rpmStart) * NUM_LEDS / static_cast<float>(rpmMax - rpmStart)) + 1;
    if (numLedsToLight > NUM_LEDS) numLedsToLight = NUM_LEDS;
  }

  strip.clear(); 

  if (redline) {
      for(int i = 0; i < NUM_LEDS; i++) {        
        strip.setPixelColor(i, strip.Color(255, 0, 0));
    }
  } else {
    for (int i = 0; i < NUM_LEDS; i++) {
      if (i >= NUM_LEDS - numLedsToLight) {
        if (i >= 6) strip.setPixelColor(i, strip.Color(0, 0, 255));       
        else if (i >= 3) strip.setPixelColor(i, strip.Color(0, 255, 0)); 
        else strip.setPixelColor(i, strip.Color(255, 85, 0));               
      }
    }
  }
  strip.show(); 
}

void playBootLedAnimation() {
  strip.clear();
  strip.show();

  for (int step = 0; step < (NUM_LEDS + 1) / 2; ++step) {
    const int leftIndex = step;
    const int rightIndex = NUM_LEDS - 1 - step;

    strip.setPixelColor(leftIndex, strip.Color(255, 0, 0));
    if (rightIndex != leftIndex) {
      strip.setPixelColor(rightIndex, strip.Color(255, 0, 0));
    }

    strip.show();
    delay(130);
  }

  delay(1100);
  strip.clear();
  strip.show();
}

void drawGauge(int cx, int cy, int radius, int thickness, float minVal, float maxVal, float val) {
    if (val < minVal) val = minVal; 
    if (val > maxVal) val = maxVal;

    float start_angle = 2.618f; 
    float end_angle = 6.807f;   
    float mid_radius = radius - (thickness / 2.0f);

    float step_out = 1.0f / radius;
    for (float a = start_angle; a <= end_angle; a += step_out) {
        u8g2.drawPixel(round(cx + radius * cos(a)), round(cy + radius * sin(a)));
    }
    float step_in = 1.0f / (radius - thickness);
    for (float a = start_angle; a <= end_angle; a += step_in) {
        u8g2.drawPixel(round(cx + (radius - thickness) * cos(a)), round(cy + (radius - thickness) * sin(a)));
    }

    u8g2.drawCircle(round(cx + mid_radius * cos(start_angle)), round(cy + mid_radius * sin(start_angle)), thickness / 2);
    u8g2.drawCircle(round(cx + mid_radius * cos(end_angle)), round(cy + mid_radius * sin(end_angle)), thickness / 2);

    for (int i = 0; i <= 4; ++i) {
        float angle = start_angle + i * ((end_angle - start_angle) / 4.0f);
        int x1 = round(cx + (radius - thickness) * cos(angle));
        int y1 = round(cy + (radius - thickness) * sin(angle));
        int x2 = round(cx + (radius - thickness - 5) * cos(angle)); 
        int y2 = round(cy + (radius - thickness - 5) * sin(angle));
        u8g2.drawLine(x1, y1, x2, y2);
    }

    float normalizedVal = (val - minVal) / (maxVal - minVal);
    float target_angle = start_angle + (normalizedVal * (end_angle - start_angle));

    for (int r = radius - thickness; r <= radius; r++) {
        float step_fill = 1.0f / r; 
        for (float a = start_angle; a <= target_angle; a += step_fill) {
            u8g2.drawPixel(round(cx + r * cos(a)), round(cy + r * sin(a)));
        }
    }

    u8g2.drawDisc(round(cx + mid_radius * cos(start_angle)), round(cy + mid_radius * sin(start_angle)), thickness / 2);
    if (normalizedVal > 0.01f) { 
        u8g2.drawDisc(round(cx + mid_radius * cos(target_angle)), round(cy + mid_radius * sin(target_angle)), thickness / 2);
    }
}

// --- SCREEN 1 ---
void drawScreen1(const VehicleData& state) {
    char textBuf[16];

    u8g2.setFontMode(1);
    u8g2.setBitmapMode(1);

    u8g2.drawFrame(0, 0, 240, 128);
    u8g2.drawFrame(60, 0, 180, 22);
    u8g2.drawFrame(60, 21, 180, 22);
    u8g2.drawLine(60, 0, 60, 128);
    u8g2.drawLine(150, 0, 150, 128);
    u8g2.drawLine(61, 84, 150, 84);

    u8g2.setFont(u8g2_font_profont17_tr);

    u8g2.drawStr(12, 15, "GEAR");
    u8g2.setFont(u8g2_font_logisoso92_tn);
    snprintf(textBuf, sizeof(textBuf), "%d", state.currentGear);
    u8g2.drawStr(0, 119, textBuf);

    u8g2.setFont(u8g2_font_profont17_tr);

    u8g2.drawStr(63, 17, "VOLT");
    snprintf(textBuf, sizeof(textBuf), "%.2f", state.batteryVolts); 
    u8g2.drawStr(104, 17, textBuf);
    
    u8g2.drawStr(63, 38, "WATR");
    snprintf(textBuf, sizeof(textBuf), "%.1f", state.engineWaterTemp);
    u8g2.drawStr(104, 38, textBuf);

    u8g2.drawStr(153, 17, "OilP");
    snprintf(textBuf, sizeof(textBuf), "%.1f", state.oilPress);
    u8g2.drawStr(199, 17, textBuf);

    u8g2.drawStr(153, 38, "OilT");
    snprintf(textBuf, sizeof(textBuf), "%.1f", state.oilTemp);
    u8g2.drawStr(199, 38, textBuf);

    u8g2.drawStr(182, 120, "BAR");
    snprintf(textBuf, sizeof(textBuf), "%.1f", state.boostPressure);
    u8g2.drawStr(182, 106, textBuf);

    u8g2.drawStr(64, 58, "RPM");
    u8g2.drawStr(64, 100, "KPH");

    u8g2.setFont(u8g2_font_profont29_tr);    

    snprintf(textBuf, sizeof(textBuf), "%d", state.rpm);
    u8g2.drawStr(64, 81, textBuf);
    
    snprintf(textBuf, sizeof(textBuf), "%d", state.speed);
    u8g2.drawStr(64, 124, textBuf);

    drawGauge(194, 84, 40, 10, 0.0, 2.5, state.boostPressure);

    if (state.hasWarning && state.warningMsg != nullptr) {
        u8g2.setDrawColor(1);
        u8g2.drawBox(60, 0, 180, 22);
        u8g2.setDrawColor(0); 
        u8g2.setFont(u8g2_font_profont17_tr);
        u8g2.drawStr(66, 17, state.warningMsg);
        u8g2.setDrawColor(1); 
    }
}

// --- SCREEN 2 ---
void drawScreen2(const VehicleData& state) {
  char textBuffer[16];
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);

  u8g2.drawFrame(0, 0, 240, 128);
  u8g2.drawLine(0, 42, 239, 42);
  u8g2.drawLine(0, 84, 239, 84);
  u8g2.drawLine(59, 0, 59, 127);
  u8g2.drawLine(119, 0, 119, 127);
  u8g2.drawLine(180, 0, 180, 127);

  u8g2.setFont(u8g2_font_t0_16b_tr);
  u8g2.drawStr(10, 15, "Oil T");
  u8g2.drawStr(69, 15, "Oil P");
  u8g2.drawStr(122, 15, "EWaterT");
  u8g2.drawStr(198, 15, "RPM");

  u8g2.setFont(u8g2_font_profont22_tr);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.oilTemp); u8g2.drawStr(5, 35, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.oilPress); u8g2.drawStr(65, 35, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.engineWaterTemp); u8g2.drawStr(125, 35, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%d", state.rpm); u8g2.drawStr(187, 35, textBuffer);

  u8g2.setFont(u8g2_font_t0_16b_tr);
  u8g2.drawStr(5, 58, "Lambda");
  u8g2.drawStr(61, 58, "IntakeT");
  u8g2.drawStr(135, 58, "EGT");
  u8g2.drawStr(186, 58, "12VOLT");

  u8g2.setFont(u8g2_font_profont22_tr);
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", state.lambdaVal); u8g2.drawStr(4, 77, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.intakeTemp); u8g2.drawStr(65, 77, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.egt); u8g2.drawStr(121, 77, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", state.batteryVolts); u8g2.drawStr(182, 77, textBuffer);

  u8g2.setFont(u8g2_font_t0_16b_tr);
  u8g2.drawStr(10, 99, "Boost");
  u8g2.drawStr(62, 99, "HybridT");
  u8g2.drawStr(123, 99, "HybridV");
  u8g2.drawStr(195, 99, "Gear");

  u8g2.setFont(u8g2_font_profont22_tr);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.boostPressure); u8g2.drawStr(11, 120, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.hybridTemp); u8g2.drawStr(66, 120, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", state.hybridVolts); u8g2.drawStr(121, 120, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%d", state.currentGear); u8g2.drawStr(205, 120, textBuffer);
}

// --- SCREEN 3 ---
void drawScreen3(const VehicleData& state) {
  char textBuffer[16];
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);
  
  u8g2.drawFrame(0, 0, 240, 128);
  u8g2.drawLine(0, 31, 239, 31);
  u8g2.drawLine(0, 63, 239, 63);
  u8g2.drawLine(0, 95, 239, 95);
  u8g2.drawLine(59, 0, 59, 127);
  u8g2.drawLine(119, 0, 119, 127);
  u8g2.drawLine(180, 0, 180, 127);

  u8g2.setFont(u8g2_font_t0_16b_tr);
  u8g2.drawStr(18, 12, "T_1"); u8g2.drawStr(77, 12, "T_2"); u8g2.drawStr(134, 12, "T_3"); u8g2.drawStr(199, 12, "T_4");

  u8g2.setFont(u8g2_font_profont22_tr);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.T_1); u8g2.drawStr(6, 30, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.T_2); u8g2.drawStr(67, 30, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.T_3); u8g2.drawStr(127, 30, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.T_4); u8g2.drawStr(188, 30, textBuffer);

  u8g2.setFont(u8g2_font_t0_16b_tr);
  u8g2.drawStr(18, 43, "T_5"); u8g2.drawStr(77, 43, "T_6"); u8g2.drawStr(134, 43, "T_7"); u8g2.drawStr(199, 43, "T_8");

  u8g2.setFont(u8g2_font_profont22_tr);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.T_5); u8g2.drawStr(6, 61, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.T_6); u8g2.drawStr(67, 61, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.T_7); u8g2.drawStr(127, 61, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.T_8); u8g2.drawStr(188, 61, textBuffer);

  u8g2.setFont(u8g2_font_t0_16b_tr);
  u8g2.drawStr(18, 75, "T_9"); u8g2.drawStr(73, 75, "T_10"); u8g2.drawStr(131, 75, "T_11"); u8g2.drawStr(196, 75, "T_12");

  u8g2.setFont(u8g2_font_profont22_tr);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.T_9); u8g2.drawStr(6, 93, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.T_10); u8g2.drawStr(67, 93, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.T_11); u8g2.drawStr(127, 93, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.T_12); u8g2.drawStr(188, 93, textBuffer);

  u8g2.setFont(u8g2_font_t0_16b_tr);
  u8g2.drawStr(15, 107, "T_13"); u8g2.drawStr(73, 107, "T_14"); u8g2.drawStr(131, 107, "T_15"); u8g2.drawStr(196, 107, "T_16");

  u8g2.setFont(u8g2_font_profont22_tr);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.T_13); u8g2.drawStr(6, 125, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.T_14); u8g2.drawStr(67, 125, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.T_15); u8g2.drawStr(127, 125, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.T_16); u8g2.drawStr(188, 125, textBuffer);
}

// --- SCREEN 4 ---
void drawScreen4(const VehicleData& state) {
  char textBuffer[16];
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);

  u8g2.drawFrame(0, 0, 240, 128);  
  u8g2.drawLine(0, 42, 239, 42);
  u8g2.drawLine(0, 84, 239, 84);
  u8g2.drawLine(59, 0, 59, 127);
  u8g2.drawLine(119, 0, 119, 127);
  u8g2.drawLine(180, 0, 180, 127);

  u8g2.setFont(u8g2_font_t0_16b_tr);
  u8g2.drawStr(15, 15, "V_1"); u8g2.drawStr(77, 15, "V_2"); u8g2.drawStr(138, 15, "V_3"); u8g2.drawStr(199, 15, "V_4");

  u8g2.setFont(u8g2_font_profont22_tr);
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", state.V_1); u8g2.drawStr(6, 35, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", state.V_2); u8g2.drawStr(67, 35, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", state.V_3); u8g2.drawStr(127, 35, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", state.V_4); u8g2.drawStr(188, 35, textBuffer);

  u8g2.setFont(u8g2_font_t0_16b_tr);
  u8g2.drawStr(15, 58, "V_5"); u8g2.drawStr(77, 58, "V_6"); u8g2.drawStr(138, 58, "V_7"); u8g2.drawStr(199, 58, "V_8");

  u8g2.setFont(u8g2_font_profont22_tr);
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", state.V_5); u8g2.drawStr(6, 77, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", state.V_6); u8g2.drawStr(67, 77, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", state.V_7); u8g2.drawStr(127, 77, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", state.V_8); u8g2.drawStr(188, 77, textBuffer);

  u8g2.setFont(u8g2_font_t0_16b_tr);
  u8g2.drawStr(15, 100, "V_9"); u8g2.drawStr(74, 100, "V_10"); u8g2.drawStr(131, 100, "V_out"); u8g2.drawStr(192, 100, "I_out");

  u8g2.setFont(u8g2_font_profont22_tr);
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", state.V_9); u8g2.drawStr(6, 121, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", state.V_10); u8g2.drawStr(67, 121, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", state.V_all); u8g2.drawStr(121, 121, textBuffer);
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.I_out); u8g2.drawStr(181, 121, textBuffer); 
}

// --- SCREEN 5: BSPD Setup Screen ---
void drawScreen5(const VehicleData& state) {
  char textBuffer[16];
  
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);
  
  // rect 1
  u8g2.drawFrame(0, 0, 240, 128);
  // line 2
  u8g2.drawLine(0, 64, 240, 64);
  // line 3
  u8g2.drawLine(80, 0, 80, 128);
  // line 4
  u8g2.drawLine(160, 0, 160, 128);
  
  // Headers
  u8g2.setFont(u8g2_font_profont22_tr);
  u8g2.drawStr(10, 20, "APP %");
  u8g2.drawStr(10, 84, "BRK P");
  u8g2.drawStr(90, 20, "APP V");
  u8g2.drawStr(90, 84, "BRK V");
  u8g2.drawStr(170, 20, "BSPD1");
  u8g2.drawStr(170, 84, "BSPD2");

  // Dynamic Value Rendering
  u8g2.setFont(u8g2_font_profont29_tr); 

  // Left Column (Live Measured Physical Values)
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.tpsPercent);
  u8g2.drawStr(10, 52, textBuffer);
  
  // Displaying pressure in bar for the UI (Dividing kPa by 100)
  snprintf(textBuffer, sizeof(textBuffer), "%.1f", state.brakePressKpa / 100.0f);
  u8g2.drawStr(10, 116, textBuffer);

  // Middle Column (Live Measured Sensor Voltages)
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", state.tpsVoltage);
  u8g2.drawStr(90, 52, textBuffer);
  
  snprintf(textBuffer, sizeof(textBuffer), "%.2f", state.brakeVoltage);
  u8g2.drawStr(90, 116, textBuffer);

  // Right Column (Captured Voltages at the exact threshold crossing)
  if (state.capturedTpsVoltage > 0.01f) {
    snprintf(textBuffer, sizeof(textBuffer), "%.2f", state.capturedTpsVoltage);
    u8g2.drawStr(170, 52, textBuffer);
  } else {
    u8g2.drawStr(170, 52, "---"); // Show dashes until a capture happens
  }
  
  if (state.capturedBrakeVoltage > 0.01f) {
    snprintf(textBuffer, sizeof(textBuffer), "%.2f", state.capturedBrakeVoltage);
    u8g2.drawStr(170, 116, textBuffer);
  } else {
    u8g2.drawStr(170, 116, "---");
  }
}

void setup() {
  delay(500); 
  
  pinMode(BACKLIGHT_PIN, OUTPUT); 
  digitalWrite(BACKLIGHT_PIN, HIGH); 
  
  // Initialize Concurrency
  stateMutex = xSemaphoreCreateMutex();
  if (stateMutex == NULL) {
    while(true); // Hard fault if OS fails to create mutex
  }

  strip.begin();
  strip.setBrightness(50);
  strip.clear();
  strip.show();
  
  u8g2.begin();
  u8g2.setContrast(150);
  
  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);
  u8g2.drawXBM(73, 10, 94, 82, SZEngine_logo);
  u8g2.drawXBM(10, 100, 221, 27, SZEngine_title);
  u8g2.sendBuffer();

  playBootLedAnimation();

  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_NORMAL);
  g_config.rx_queue_len = 20; 
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); 
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  
  // Safe Initialization Trap
  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK || twai_start() != ESP_OK) {
    while (true) {
      strip.fill(strip.Color(255, 0, 0)); // Flash Red to indicate fatal CAN failure
      strip.show();
      vTaskDelay(pdMS_TO_TICKS(500));
      strip.clear();
      strip.show();
      vTaskDelay(pdMS_TO_TICKS(500));
    }
  }

  // --- Start CORE 0 ---
  xTaskCreatePinnedToCore(
    TaskCANcode,   /* Task function */
    "TaskCAN",     /* Name of task */
    4096,          /* Stack size of task */
    NULL,          /* Parameter of the task */
    10,            /* Priority of the task */
    &TaskCAN,      /* Task handle */
    0);            /* Pin task to core 0 */

  Serial.begin(115200);
  
  globalVehicleState.hasWarning = false;
  globalVehicleState.warningMsg = nullptr;
}

// --- WARNING EVALUATOR (Runs in the UI Thread) ---
void evaluateWarnings(VehicleData& state) {
  static bool popupTriggered[8] = {false};
  static unsigned long popupTimer[8] = {0};
  const unsigned long POPUP_DURATION = 3000; 

  unsigned long currentMillis = millis();
  
  state.hasWarning = false;
  state.warningMsg = nullptr;

  const char* efuseNames[8] = {
    "HYBRID", "VENT 1", "VENT 2", "IGN/INJ", 
    "FUELPUMP", "WATER P1", "WATER P2", "12V AUX"
  };
  
  for (int i = 0; i < 8; i++) {
    if (state.efuseFaultActive[i]) {
      if (!popupTriggered[i]) {
        popupTriggered[i] = true;         
        popupTimer[i] = currentMillis;    
      }
    } else {
      popupTriggered[i] = false;
    }

    if (popupTriggered[i] && (currentMillis - popupTimer[i] < POPUP_DURATION)) {
      static char efuseMsg[24];
      snprintf(efuseMsg, sizeof(efuseMsg), "ERR: %s FAULT", efuseNames[i]);
      state.hasWarning = true;
      state.warningMsg = efuseMsg;
      
      return; 
    }
  }

  if (state.pduFetError) {
    state.hasWarning = true;
    state.warningMsg = "ERR: PDU FET FAULT";
  } else if (state.pduVoltError) {
    state.hasWarning = true;
    state.warningMsg = "ERR: PDU LOW VOLT";
  } else if (state.pduPowerError) {
    state.hasWarning = true;
    state.warningMsg = "ERR: PDU PWR CALC";
  }
}

void loop() {
  // --- CORE 1: Screen render ---
  if (millis() - lastScreenUpdate >= 33) {
    lastScreenUpdate = millis();

    VehicleData localState; 

    if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      localState = globalVehicleState; 
      xSemaphoreGive(stateMutex);
    } else {
      return; 
    }

    evaluateWarnings(localState);
    updateLEDs(localState.rpm);
    u8g2.clearBuffer();          
    
    if (localState.activeScreen == 1) {
      drawScreen1(localState); 
    } else if (localState.activeScreen == 2) {
      drawScreen2(localState);
    } else if (localState.activeScreen == 3) {
      drawScreen3(localState);
    } else if (localState.activeScreen == 4) {
      drawScreen4(localState);
    } else if (localState.activeScreen == 5) {
      drawScreen5(localState); 
    }
    
    u8g2.sendBuffer();          
  }
}