#include <Arduino.h>
#include "config.h"
#include "LEDController.h"
#include "ESPNowManager.h"
#include "GameManager.h"
#include "Logger.h"

// ==================== GLOBAL VARIABLES ====================
// Definizione broadcastAddress (dichiarato extern in config.h)
uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// ==================== GLOBAL OBJECTS ====================
LEDController leds(LED_PIN, NUM_LEDS);
ESPNowManager espNow;
GameManager* gameManager = nullptr;

// ==================== BUTTON HANDLING ====================
volatile bool buttonFlag = false;
unsigned long lastButtonTime = 0;

void IRAM_ATTR buttonISR() {
    buttonFlag = true;
}

// ==================== ESP-NOW CALLBACK ====================
void onMessageReceived(const Message& msg, const uint8_t* macAddr) {
    if (gameManager != nullptr) {
        gameManager->handleMessage(msg, macAddr);
    }
}

// ==================== SETUP ====================
void setup() {
    Serial.begin(115200);
    delay(1000);

    // Inizializza Logger
    Log.begin(Serial, LOG_INFO);  // LOG_DEBUG per più dettagli

    Log.info("\n====================================");
    Log.info("       PRENOTOMETRO v1.0");
    Log.info("====================================");
    Log.info("Device Mode: %s", IS_MASTER ? "MASTER" : "SLAVE");

    if (!IS_MASTER) {
        Log.info("Slave ID: %d", SLAVE_ID);
        const char* colorName;
        switch (SLAVE_ID) {
            case 0: colorName = "YELLOW"; break;
            case 1: colorName = "GREEN"; break;
            case 2: colorName = "BLUE"; break;
            case 3: colorName = "RED"; break;
            default: colorName = "UNKNOWN"; break;
        }
        Log.info("Color: %s", colorName);
    }

    Log.info("====================================\n");

    // Inizializza LED
    Log.info("Initializing LEDs...");
    leds.begin();
    leds.setColor(COLOR_OFF);

    // Inizializza pulsante
    Log.info("Initializing button...");
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

    // Inizializza ESP-NOW
    Log.info("Initializing ESP-NOW...");
    if (!espNow.begin()) {
        Log.error("ESP-NOW initialization failed!");
        leds.setColor(COLOR_RED);
        while (1) {
            delay(100);
        }
    }

    // Registra callback ESP-NOW
    espNow.setMessageCallback(onMessageReceived);

    // Crea GameManager
    Log.info("Initializing GameManager...");
    gameManager = new GameManager(leds, espNow, IS_MASTER, SLAVE_ID);
    gameManager->begin();

    Log.info("\n=== SETUP COMPLETE ===\n");

    // Test LED iniziale
    leds.setColor(IS_MASTER ? COLOR_BLUE : SLAVE_COLORS[SLAVE_ID]);
    delay(1000);
    leds.setColor(COLOR_OFF);
}

// ==================== LOOP ====================
void loop() {
    // Gestisci pressione pulsante
    if (buttonFlag) {
        buttonFlag = false;
        unsigned long now = millis();

        // Debounce software aggiuntivo
        if (now - lastButtonTime > BUTTON_DEBOUNCE_MS) {
            lastButtonTime = now;
            gameManager->handleButtonPress();
        }
    }

    // Aggiorna game manager
    if (gameManager != nullptr) {
        gameManager->update();
    }

    // Piccolo delay per non saturare la CPU
    delay(10);
}
