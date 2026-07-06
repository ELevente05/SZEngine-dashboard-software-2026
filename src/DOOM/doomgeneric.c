#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_heap_caps.h>
#include "doomkeys.h"
#include "m_argv.h"
#include "doomdef.h"
#include "doomstat.h"
#include "doomgeneric.h"
#include "i_video.h"

pixel_t* DG_ScreenBuffer = NULL;
byte* I_VideoBuffer = NULL;

void M_FindResponseFile(void);
void D_DoomMain (void);

static void *DoomGenericAlloc(size_t size)
{
    void *ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    if (ptr == NULL) {
        ptr = heap_caps_malloc(size, MALLOC_CAP_INTERNAL);
    }
    if (ptr == NULL) {
        ptr = heap_caps_malloc(size, MALLOC_CAP_DEFAULT);
    }
    return ptr;
}

void doomgeneric_Create(int argc, char **argv)
{
    myargc = argc;
    myargv = argv;
    M_FindResponseFile();

    // 1. Allocate DOOM's 32-bit screen buffer.
    DG_ScreenBuffer = (pixel_t*)DoomGenericAlloc(DOOMGENERIC_RESX * DOOMGENERIC_RESY * sizeof(*DG_ScreenBuffer));

    // 2. Allocate DOOM's 8-bit internal drawing buffer.
    I_VideoBuffer = (byte*)DoomGenericAlloc(DOOMGENERIC_RESX * DOOMGENERIC_RESY);

    // 3. Allocate the openings array used by the renderer.
    extern short* openings;
    openings = (short*)DoomGenericAlloc(20480 * sizeof(short));

    if (DG_ScreenBuffer == NULL || I_VideoBuffer == NULL || openings == NULL) {
        while (true) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

    DG_Init();
    D_DoomMain();
}

// =====================================================================
// DOOM ENGINE TICK (Runs once every 33ms alongside your dashboard loop)
// =====================================================================
void doomgeneric_Tick()
{
    extern void D_ProcessEvents(void);
    extern void G_Ticker(void);
    extern void I_FinishUpdate(void);
    
    D_ProcessEvents();
    G_Ticker();
    I_FinishUpdate();
}

// =====================================================================
// GRAPHICS & PALETTE TRANSLATION
// =====================================================================
static uint32_t s_palette[256];

void I_SetPalette(byte* palette)
{
    for (int i = 0; i < 256; i++) {
        // Convert DOOM's 8-bit RGB palette to 32-bit colors
        s_palette[i] = (palette[0] << 16) | (palette[1] << 8) | palette[2];
        palette += 3;
    }
}

void I_FinishUpdate(void)
{
    // 1. Translate the 8-bit game screen to our 32-bit color buffer
    for (int i = 0; i < DOOMGENERIC_RESX * DOOMGENERIC_RESY; i++) {
        DG_ScreenBuffer[i] = s_palette[I_VideoBuffer[i]];
    }
    
    // 2. Push the translated frame to your dashboard's DG_DrawFrame()!
    DG_DrawFrame();
}

int I_GetPaletteIndex(int r, int g, int b) 
{
    // We handle our own color translation for the dashboard, 
    // so we can safely return 0 to satisfy the compiler.
    return 0;
}

// Stubs required by the engine, safely ignored on the ESP32
void I_InitGraphics(void) 
{
    // This port uses I_VideoBuffer directly as the engine drawing buffer.
    // No separate screens array is needed here.
    if (I_VideoBuffer == NULL) {
        I_VideoBuffer = (byte*)DoomGenericAlloc(DOOMGENERIC_RESX * DOOMGENERIC_RESY);
    }
}
void I_ShutdownGraphics(void) { }
void I_WaitVBL(int count) { }
void I_BeginRead(void) { }
void I_EndRead(void) { }
void I_UpdateNoBlit(void) { }
void I_ReadScreen(byte* scr) { }
// --- MISSING STUBS TO SATISFY THE LINKER ---
void I_StartTic(void) {}
void I_StartFrame(void) {}
void I_SetWindowTitle(char *title) {}
void I_GraphicsCheckCommandLine(void) {}
void I_SetGrabMouseCallback(grabmouse_callback_t func) {}
void I_EnableLoadingDisk(void) {}
void I_DisplayFPSDots(boolean dots_on) {}
void I_CheckIsScreensaver(void) {}
void I_BindVideoVariables(void) {}

// Global variables the engine expects to exist
boolean screenvisible = true;
boolean screensaver_mode = false;
int usegamma = 0;
int usemouse = 0;
float mouse_acceleration = 0.0f;
int mouse_threshold = 0;
// Audio entry points are provided by the DOOM sound implementation.
// No extra stubs are needed here.