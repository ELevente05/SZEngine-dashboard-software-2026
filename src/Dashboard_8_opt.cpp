#include <Arduino.h>
#include <U8g2lib.h>
#include <LittleFS.h>
#include "driver/twai.h"
#include "doomkeys.h"

// =========================================================================
// 1. HARDWARE CONFIGURATION
// =========================================================================
#define SPI_SCK   12
#define SPI_MOSI  11
#define SPI_CS    10
#define SPI_DC    9
#define SPI_RESET 8

#define CAN_TX_PIN 47
#define CAN_RX_PIN 48

// Initialize the EA DOGXL240-7 Display
U8G2_UC1611_EA_DOGXL240_F_4W_SW_SPI u8g2(U8G2_R2, SPI_SCK, SPI_MOSI, SPI_CS, SPI_DC, SPI_RESET);

// =========================================================================
// 2. DOOM ENGINE BRIDGE & VARIABLES
// =========================================================================
extern "C" {
    #include "doomgeneric.h"
}

// Memory pointer to DOOM's internal color graphics
extern uint32_t* DG_ScreenBuffer; 

// Variables shared between Core 0 (CAN) and Core 1 (DOOM)
volatile uint8_t currentDoomButtons = 0;
volatile uint8_t currentDoomDIP = 0;
uint8_t lastDoomButtons = 0;

// =========================================================================
// 3. CORE 0: CAN BUS LISTENER TASK
// =========================================================================
void TaskCANcode(void * pvParameters) {
    twai_message_t rx_msg;
    for(;;) {
        // Listen for the steering wheel's DOOM controller broadcast (0x666)
        if (twai_receive(&rx_msg, pdMS_TO_TICKS(1)) == ESP_OK) {
            if (rx_msg.identifier == 0x666) {
                currentDoomButtons = rx_msg.data[0];
                currentDoomDIP = rx_msg.data[1];
            }
        }
    }
}

// =========================================================================
// 4. DOOMGENERIC IMPLEMENTATION (The 5 Required Functions)
// =========================================================================

void DG_Init() {
    // We handle hardware initialization in our standard Arduino setup()
}

void DG_DrawFrame() {
    u8g2.clearBuffer();

    // Downscale DOOM's 320x200 32-bit color frame to 240x128 1-bit monochrome
    for (int y = 0; y < 128; y++) {
        int srcY = (y * 200) / 128; 
        
        for (int x = 0; x < 240; x++) {
            int srcX = (x * 320) / 240;
            uint32_t color = DG_ScreenBuffer[srcY * 320 + srcX];

            uint8_t r = (color >> 16) & 0xFF;
            uint8_t g = (color >> 8) & 0xFF;
            uint8_t b = color & 0xFF;

            // Calculate luminance and apply threshold for black & white
            uint8_t luminance = (r * 77 + g * 150 + b * 29) >> 8;
            if (luminance > 75) { 
                u8g2.drawPixel(x, y);
            }
        }
    }
    u8g2.sendBuffer();
}

int DG_GetKey(int* pressed, unsigned char* doomKey) {
    uint8_t changedButtons = currentDoomButtons ^ lastDoomButtons;
    
    if (changedButtons == 0) return 0; 

    for (int i = 0; i < 8; i++) {
        if (changedButtons & (1 << i)) {
            *pressed = (currentDoomButtons & (1 << i)) ? 1 : 0;
            
            switch (i) {
                case 0: *doomKey = KEY_UPARROW; break;    // Front 1: Forward
                case 1: *doomKey = KEY_DOWNARROW; break;  // Front 2: Backward
                case 2: *doomKey = ','; break;            // Front 3: Strafe Left
                case 3: *doomKey = '.'; break;            // Front 4: Strafe Right
                case 4: *doomKey = ' '; break;            // Front 5: Use
                case 5: *doomKey = KEY_RCTRL; break;      // Front 6: Fire
                case 6: *doomKey = KEY_LEFTARROW; break;  // Back 1: Turn Left
                case 7: *doomKey = KEY_RIGHTARROW; break; // Back 2: Turn Right
            }
            lastDoomButtons ^= (1 << i);
            return 1; 
        }
    }
    return 0;
}

void DG_SetWindowTitle(const char * title) {
    // Unused on embedded systems
}

void DG_SleepMs(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

uint32_t DG_GetTicksMs() {
    return millis();
}

// =========================================================================
// 5. MAIN ARDUINO SETUP & LOOP
// =========================================================================
void setup() {
    Serial.begin(115200);
    Serial.println("Starting Dashboard DOOM...");

    // 1. Start Display
    u8g2.begin();
    u8g2.setContrast(150);

    // 2. Start LittleFS (Hard Drive)
    if (!LittleFS.begin()) {
        Serial.println("Failed to mount LittleFS! Check partition table.");
        while (true) delay(100); // Halt if drive fails
    }

    // 3. Start CAN Bus
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); 
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    twai_driver_install(&g_config, &t_config, &f_config);
    twai_start();

    // 4. Pin the CAN listener to Core 0
    xTaskCreatePinnedToCore(TaskCANcode, "TaskCAN", 4096, NULL, 10, NULL, 0);

    // 5. Boot the DOOM Engine!
    // We act like a computer terminal and pass the file path directly to the engine
    const char* argv[] = {"doom", "-iwad", "/littlefs/doom1.wad"};
    
    // Launch the game with 3 arguments (the executable name, the flag, and the file path)
    doomgeneric_Create(3, (char**)argv);
}

void loop() {
    // DOOM's heartbeat. This runs forever on Core 1, pulling inputs and pushing frames.
    doomgeneric_Tick();
}