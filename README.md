# 🎮 Prenotometro

Sistema di prenotazione basato su ESP32-S2 Mini con comunicazione ESP-NOW.

## 📝 Descrizione

Gioco con 5 dispositivi ESP32-S2 Mini:
- **1 Master** (controller di gioco)
- **4 Slave** (pulsanti colorati: 🟡 Giallo, 🟢 Verde, 🔵 Blu, 🔴 Rosso)

### Funzionamento

1. **Connessione**: gli slave si connettono automaticamente al master con retry ogni 2s. Il master mostra l'arcobaleno finché nessuno è connesso, poi cicla i colori degli slave connessi
2. **Ready**: quando tutti e 4 gli slave sono connessi, il master è pronto (LED spenti)
3. **Start Game**: il master preme il pulsante, tutti i LED diventano verdi 🟢
4. **Prenotazione**: il primo slave che preme il pulsante vince
5. **Vittoria**: tutti i dispositivi mostrano il colore del vincitore (pulse sul master)
6. **Reset**: il master preme il pulsante per tornare al punto 3

### Falsa partenza

Se uno slave preme il pulsante prima che il gioco sia partito, tutti i dispositivi lampeggiano rosso 3 volte e tornano in attesa.

### Keepalive

Gli slave inviano un heartbeat ogni 3 secondi. Se il master non riceve heartbeat da uno slave per 10 secondi, lo considera disconnesso, annulla il round e torna in attesa delle connessioni.

## 🛠️ Hardware

- **Microcontroller**: ESP32-S2 Mini (x5)
- **LED**: WS2812B (14 LED per dispositivo)
- **Pulsante**: Push button

### Pin utilizzati

| Pin | Funzione | Note |
|-----|----------|------|
| `GPIO18` | LED WS2812B (Data In) | Configurabile in platformio.ini |
| `GPIO33` | Pulsante | Pull-up interno, configurabile in platformio.ini |

## 📦 Software

- **PlatformIO** + Arduino Framework
- **ESP-NOW** per comunicazione wireless
- **Adafruit NeoPixel** per controllo LED

### Architettura

```
src/
├── config.h         # Configurazione pin, colori, timing, messaggi
├── LEDController    # Gestione LED WS2812B (colori, pulse, rainbow)
├── ESPNowManager    # Comunicazione ESP-NOW (send, receive, peer management)
├── GameManager      # Logica del gioco (stati, connessione, heartbeat)
├── Logger           # Logging seriale colorato
└── main.cpp         # Entry point (setup/loop, test mode)
```

### Messaggi ESP-NOW

| Tipo | Codice | Direzione | Descrizione |
|------|--------|-----------|-------------|
| `CONNECT_REQUEST` | 0x01 | Slave → Master | Richiesta connessione |
| `CONNECT_ACK` | 0x02 | Master → Slave | Conferma connessione |
| `START_GAME` | 0x03 | Master → All | Avvia gioco |
| `BUTTON_PRESSED` | 0x04 | Slave → Master | Pulsante premuto |
| `WINNER_ANNOUNCE` | 0x05 | Master → All | Annuncio vincitore |
| `HEARTBEAT` | 0x06 | Slave → Master | Keepalive |
| `FALSE_START` | 0x07 | Bidirezionale | Falsa partenza |

## 🚀 Build & Upload

```bash
# Compila e upload (modalità gioco)
pio run -e ESP32-S2-Production -t upload

# Monitor seriale
pio device monitor
```

## 🧪 Test Mode

Modalità per testare i LED RGB senza bisogno di più dispositivi.

```bash
# Compila e upload in test mode
pio run -e testmode -t upload
```

**Comportamento:**
1. All'avvio: 3 lampeggi verdi veloci
2. Attesa pressione pulsante
3. Cicla 7 colori (Rosso, Verde, Blu, Giallo, Ciano, Magenta, Bianco), 5 secondi ciascuno
4. Al termine, torna in attesa del pulsante

## 📡 Configurazione Master/Slave

Modificare in `config.h`:
```cpp
#define IS_MASTER true  // false per slave
#define SLAVE_ID 0      // 0-3 per gli slave
```

I pin possono essere sovrascritti nei build flags di `platformio.ini`:
```ini
build_flags =
    -D LED_PIN=18
    -D BUTTON_PIN=33
    -D NUM_LEDS=14
```

## 🎨 Colori Slave

| Slave ID | Colore | Hex |
|----------|--------|---------|
| 0 | 🟡 Giallo | `#FFFF00` |
| 1 | 🟢 Verde  | `#00FF00` |
| 2 | 🔵 Blu    | `#0000FF` |
| 3 | 🔴 Rosso  | `#FF0000` |
