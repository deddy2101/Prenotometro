#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"

class LEDController {
public:
    LEDController(uint8_t pin, uint16_t numLeds);

    void begin();
    void setColor(uint32_t color);
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void setBrightness(uint8_t brightness);
    void clear();
    void update();

    // Animazioni
    void pulse(uint32_t color, uint16_t duration);
    void rainbow(uint16_t duration);
    void cycleColors(uint32_t* colors, uint8_t numColors, uint16_t intervalMs);
    void spinner(uint32_t color, uint16_t speedMs);
    void blink(uint32_t color, uint16_t intervalMs);

    // Utility
    uint32_t getColor(uint8_t r, uint8_t g, uint8_t b);

private:
    Adafruit_NeoPixel* strip;
    uint16_t numLeds;

    // Variabili per animazioni
    unsigned long lastUpdate;
    uint16_t animationStep;
};

#endif // LED_CONTROLLER_H
