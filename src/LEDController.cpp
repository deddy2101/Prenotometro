#include "LEDController.h"

LEDController::LEDController(uint8_t pin, uint16_t numLeds) {
    this->numLeds = numLeds;
    this->strip = new Adafruit_NeoPixel(numLeds, pin, NEO_GRB + NEO_KHZ800);
    this->lastUpdate = 0;
    this->animationStep = 0;
}

void LEDController::begin() {
    strip->begin();
    strip->setBrightness(255);  // Massima luminosità (0-255)
    clear();
    strip->show();
}

void LEDController::setColor(uint32_t color) {
    for (uint16_t i = 0; i < numLeds; i++) {
        strip->setPixelColor(i, color);
    }
    strip->show();
}

void LEDController::setColor(uint8_t r, uint8_t g, uint8_t b) {
    setColor(strip->Color(r, g, b));
}

void LEDController::setBrightness(uint8_t brightness) {
    strip->setBrightness(brightness);
    strip->show();
}

void LEDController::clear() {
    strip->clear();
    strip->show();
}

void LEDController::update() {
    strip->show();
}

// Effetto pulsante (fade in/out)
void LEDController::pulse(uint32_t color, uint16_t duration) {
    unsigned long now = millis();
    if (now - lastUpdate > 20) {  // Aggiorna ogni 20ms
        lastUpdate = now;

        // Calcola brightness con seno (0-255)
        float phase = (now % duration) / (float)duration;
        uint8_t brightness = (sin(phase * 2 * PI) * 127) + 128;

        strip->setBrightness(brightness);
        setColor(color);
    }
}

// Effetto arcobaleno
void LEDController::rainbow(uint16_t duration) {
    unsigned long now = millis();
    if (now - lastUpdate > duration / 256) {
        lastUpdate = now;
        animationStep = (animationStep + 1) % 256;

        for (uint16_t i = 0; i < numLeds; i++) {
            uint16_t hue = ((i * 256 / numLeds) + animationStep) % 256;
            strip->setPixelColor(i, strip->ColorHSV(hue * 256));
        }
        strip->show();
    }
}

// Cicla tra diversi colori
void LEDController::cycleColors(uint32_t* colors, uint8_t numColors, uint16_t intervalMs) {
    unsigned long now = millis();
    if (now - lastUpdate >= intervalMs) {
        lastUpdate = now;
        animationStep = (animationStep + 1) % numColors;
        setColor(colors[animationStep]);
    }
}

uint32_t LEDController::getColor(uint8_t r, uint8_t g, uint8_t b) {
    return strip->Color(r, g, b);
}
