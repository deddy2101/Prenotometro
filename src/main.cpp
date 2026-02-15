#include <Arduino.h>
#include "config.h"
#include "LEDController.h"
#include "Logger.h"

#ifndef TEST_MODE
#include "ESPNowManager.h"
#include "GameManager.h"
#endif

// ==================== GLOBAL VARIABLES ====================
// Definizione broadcastAddress (dichiarato extern in config.h)
uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// ==================== GLOBAL OBJECTS ====================
LEDController leds(LED_PIN, NUM_LEDS);

#ifndef TEST_MODE
ESPNowManager espNow;
GameManager* gameManager = nullptr;
#endif

// ==================== BUTTON HANDLING ====================
volatile bool buttonFlag = false;
unsigned long lastButtonTime = 0;

void IRAM_ATTR buttonISR() {
    buttonFlag = true;
}

// ==================== CHARGING STATE ====================
enum ChargeState {
    CHARGE_NONE,        // Non in ricarica (batteria)
    CHARGE_CHARGING,    // In carica (verde lampeggia)
    CHARGE_COMPLETE     // Carica completa (verde fisso)
};

ChargeState chargeState = CHARGE_NONE;
unsigned long lastChargeSample = 0;
uint8_t greenHighCount = 0;     // Quante volte il verde è HIGH negli ultimi campioni
uint8_t chargeSampleIndex = 0;  // Indice campione corrente
bool wasCharging = false;        // Per rilevare transizione carica → non carica

// Aggiorna lo stato di ricarica (non bloccante, da chiamare nel loop)
// Ritorna true se siamo in ricarica (il gioco non deve girare)
bool updateChargeState() {
    unsigned long now = millis();

    if (now - lastChargeSample < CHARGE_SAMPLE_INTERVAL_MS) {
        return chargeState != CHARGE_NONE;
    }
    lastChargeSample = now;

    bool greenHigh = digitalRead(CHARGE_PIN_GREEN) == HIGH;
    bool blueHigh = digitalRead(CHARGE_PIN_BLUE) == HIGH;

    // Aggiorna contatore campioni verde
    if (chargeSampleIndex < CHARGE_SAMPLE_COUNT) {
        if (greenHigh) greenHighCount++;
        chargeSampleIndex++;
    } else {
        // Finestra di campionamento completata, determina stato
        ChargeState newState;

        if (greenHighCount == 0) {
            // Verde mai alto → non in ricarica
            newState = CHARGE_NONE;
        } else if (greenHighCount >= CHARGE_SAMPLE_COUNT) {
            // Verde sempre alto → carica completa
            newState = CHARGE_COMPLETE;
        } else {
            // Verde a volte alto → lampeggia → in carica
            newState = CHARGE_CHARGING;
        }

        if (newState != chargeState) {
            chargeState = newState;
            switch (chargeState) {
                case CHARGE_NONE:
                    Log.info("Charge: disconnected");
                    break;
                case CHARGE_CHARGING:
                    Log.info("Charge: charging...");
                    break;
                case CHARGE_COMPLETE:
                    Log.info("Charge: complete!");
                    break;
            }
        }

        // Reset finestra
        greenHighCount = 0;
        chargeSampleIndex = 0;
    }

    return chargeState != CHARGE_NONE;
}

void initChargePins() {
    pinMode(CHARGE_PIN_GREEN, INPUT);
    pinMode(CHARGE_PIN_BLUE, INPUT);
}

#ifdef TEST_MODE
// ==================== TEST MODE ====================

// Colori da testare: nome e valore
struct TestColor {
    const char* name;
    uint32_t color;
};

const TestColor TEST_COLORS[] = {
    {"ROSSO",     0xFF0000},
    {"VERDE",     0x00FF00},
    {"BLU",       0x0000FF},
    {"GIALLO",    0xFFFF00},
    {"CIANO",     0x00FFFF},
    {"MAGENTA",   0xFF00FF},
    {"BIANCO",    0xFFFFFF},
};
const uint8_t NUM_TEST_COLORS = sizeof(TEST_COLORS) / sizeof(TEST_COLORS[0]);

bool testRunning = false;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Log.begin(Serial, LOG_INFO);

    Log.info("\n====================================");
    Log.info("       PRENOTOMETRO - TEST MODE");
    Log.info("====================================");
    Log.info("LED Pin: %d  |  Num LEDs: %d", LED_PIN, NUM_LEDS);
    Log.info("====================================\n");

    // Inizializza LED
    leds.begin();
    leds.setColor(COLOR_OFF);

    // Inizializza pulsante
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

    // Inizializza pin ricarica
    initChargePins();

    // 3 lampeggi verdi veloci
    for (int i = 0; i < 3; i++) {
        leds.setColor(COLOR_GREEN);
        delay(200);
        leds.setColor(COLOR_OFF);
        delay(200);
    }

    Log.info("Premi il pulsante per avviare il test...");
}

void loop() {
    // Controlla stato ricarica
    if (updateChargeState()) {
        if (chargeState == CHARGE_CHARGING) {
            leds.spinner(COLOR_GREEN, 80);
        } else if (chargeState == CHARGE_COMPLETE) {
            leds.blink(COLOR_GREEN, 500);
        }
        delay(10);
        return;  // Non eseguire logica test durante la ricarica
    }

    if (buttonFlag) {
        buttonFlag = false;
        unsigned long now = millis();

        if (now - lastButtonTime > BUTTON_DEBOUNCE_MS) {
            lastButtonTime = now;

            if (!testRunning) {
                testRunning = true;
                Log.info("=== TEST LED AVVIATO ===");

                for (uint8_t i = 0; i < NUM_TEST_COLORS; i++) {
                    Log.info("[%d/%d] Colore: %s (0x%06X)", i + 1, NUM_TEST_COLORS,
                             TEST_COLORS[i].name, TEST_COLORS[i].color);
                    leds.setColor(TEST_COLORS[i].color);
                    delay(5000);
                }

                leds.setColor(COLOR_OFF);
                Log.info("=== TEST COMPLETATO ===");
                Log.info("Premi il pulsante per ripetere il test...");
                testRunning = false;
            }
        }
    }

    delay(10);
}

#else
// ==================== NORMAL MODE ====================

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

    // Inizializza pin ricarica
    Log.info("Initializing charge pins...");
    initChargePins();

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
    // Controlla stato ricarica
    if (updateChargeState()) {
        if (chargeState == CHARGE_CHARGING) {
            leds.spinner(COLOR_GREEN, 80);
        } else if (chargeState == CHARGE_COMPLETE) {
            leds.blink(COLOR_GREEN, 500);
        }
        delay(10);
        return;  // Non eseguire logica gioco durante la ricarica
    }

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

#endif // TEST_MODE
