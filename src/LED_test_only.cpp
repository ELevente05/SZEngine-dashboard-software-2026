#include <Adafruit_NeoPixel.h>

#define NUM_LEDS 9
#define LED_PIN  7

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.setBrightness(50);
  strip.show(); 
}

void loop() {
  colorWipe(strip.Color(255, 0, 0), 100); 
  colorWipe(strip.Color(0, 255, 0), 100); 
  colorWipe(strip.Color(0, 0, 255), 100); 
  
  clearStrip();
  delay(500);
}

void colorWipe(uint32_t color, int wait) {
  for(int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, color);
    strip.show();
    delay(wait);
  }
}

void clearStrip() {
  for(int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
  strip.show();
}