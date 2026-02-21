#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==================== CONFIGURAZIONE DISPOSITIVO ====================
// Cambia questo valore per configurare Master o Slave
#define IS_MASTER false  // true = Master, false = Slave

// ID Slave (solo per slave, ignorato se IS_MASTER = true)
// 0 = Giallo, 1 = Verde, 2 = Blu, 3 = Rosso
#define SLAVE_ID 3

// ==================== PIN CONFIGURATION ====================
#ifndef LED_PIN
#define LED_PIN 18      // GPIO per LED WS2812B
#endif

#ifndef BUTTON_PIN
#define BUTTON_PIN 33    // GPIO per pulsante
#endif

#ifndef NUM_LEDS
#define NUM_LEDS 14      // Numero di LED WS2812B
#endif

// Pin lettura stato ricarica (dal modulo caricabatterie)
#ifndef CHARGE_PIN_GREEN
#define CHARGE_PIN_GREEN 5   // LED verde caricatore (HIGH = in carica o completa)
#endif

#ifndef CHARGE_PIN_BLUE
#define CHARGE_PIN_BLUE 6    // LED blu caricatore (HIGH = scarica con carico)
#endif

// ==================== COLORI ====================
#define COLOR_OFF       0x000000  // Spento
#define COLOR_GREEN     0x00FF00  // Verde (gioco in corso)
#define COLOR_PINK      0xFF0080  // Rosa (start game)
#define COLOR_YELLOW    0xFFFF00  // Giallo (Slave 0)
#define COLOR_LIME      0x00FF00  // Verde Lime (Slave 1)
#define COLOR_BLUE      0x0000FF  // Blu (Slave 2)
#define COLOR_RED       0xFF2000  // Arancione rossastro (Slave 3)

// Array di colori per gli slave
const uint32_t SLAVE_COLORS[] = {
    COLOR_YELLOW,   // Slave 0
    COLOR_LIME,     // Slave 1
    COLOR_BLUE,     // Slave 2
    COLOR_RED       // Slave 3
};

// ==================== ESP-NOW CONFIGURATION ====================
#define MAX_SLAVES 4
#define ESP_NOW_CHANNEL 1
#define ESP_NOW_SEND_TIMEOUT 1000  // ms

// Indirizzo broadcast per ESP-NOW (dichiarato extern, definito in main.cpp)
extern uint8_t broadcastAddress[6];

// ==================== MESSAGGI ESP-NOW ====================
enum MessageType {
    MSG_CONNECT_REQUEST = 0x01,   // Slave -> Master: richiesta connessione
    MSG_CONNECT_ACK = 0x02,       // Master -> Slave: conferma connessione
    MSG_START_GAME = 0x03,        // Master -> All: avvia gioco
    MSG_BUTTON_PRESSED = 0x04,    // Slave -> Master: pulsante premuto
    MSG_WINNER_ANNOUNCE = 0x05,   // Master -> All: annuncio vincitore
    MSG_HEARTBEAT = 0x06,         // Slave -> Master: keepalive
    MSG_FALSE_START = 0x07,       // Falsa partenza: qualcuno ha premuto troppo presto
    MSG_MASTER_HEARTBEAT = 0x08   // Master -> All: keepalive durante il gioco
};

// Struttura messaggio ESP-NOW
struct Message {
    uint8_t type;           // Tipo di messaggio (MessageType)
    uint8_t slaveId;        // ID dello slave (0-3)
    uint8_t data;           // Dato aggiuntivo
    uint32_t timestamp;     // Timestamp messaggio
};

// ==================== GAME STATES ====================
enum GameState {
    STATE_INIT,                 // Inizializzazione
    STATE_WAITING_CONNECTIONS,  // Master: aspetta connessioni slave
    STATE_WAITING_START,        // Slave: aspetta start game
    STATE_READY,                // Tutti connessi, pronto per partire
    STATE_GAME_RUNNING,         // Gioco in corso (LED verdi)
    STATE_WINNER_ANNOUNCED      // Vincitore annunciato
};

// ==================== TIMING ====================
#define BUTTON_DEBOUNCE_MS 50         // Debounce pulsante
#define CONNECTION_CYCLE_MS 500       // Ciclo animazione connessione
#define GAME_START_DELAY_MS 3000      // Delay prima di start game
#define CONNECT_RETRY_MS 2000         // Retry connessione slave ogni 2s
#define HEARTBEAT_INTERVAL_MS 3000        // Heartbeat slave ogni 3s
#define HEARTBEAT_TIMEOUT_MS 10000        // Slave disconnesso se nessun heartbeat per 10s
#define MASTER_HEARTBEAT_INTERVAL_MS 2000 // Heartbeat master -> slave ogni 2s
#define CHARGE_SAMPLE_INTERVAL_MS 100 // Campionamento pin ricarica ogni 100ms
#define CHARGE_SAMPLE_COUNT 10        // Numero campioni per decidere stato (1s di finestra)

#endif // CONFIG_H
