# 📚 Setup e Configurazione Prenotometro

## 🔧 Configurazione Hardware

### Connessioni per ogni ESP32-S2 Mini

| Componente | Pin ESP32-S2 | Note |
|------------|--------------|------|
| LED WS2812B (Data In) | GPIO18 | Usa resistenza 330Ω |
| Pulsante | GPIO0 | Pull-up interno attivo |
| LED WS2812B (VCC) | 5V | Alimentazione LED |
| LED WS2812B (GND) | GND | Massa |

### Schema Pulsante

```
GPIO0 ----[ BUTTON ]---- GND
            (pull-up interno)
```

### Schema LED WS2812B

```
GPIO18 --[330Ω]-- DIN (WS2812B)
5V -------------- VCC
GND ------------- GND
```

## ⚙️ Configurazione Software

### 1. Configura ogni dispositivo

Apri [src/config.h](src/config.h) e modifica:

#### Per il **MASTER**:
```cpp
#define IS_MASTER true
#define SLAVE_ID 0  // Ignorato per master
```

#### Per gli **SLAVE**:
```cpp
#define IS_MASTER false
#define SLAVE_ID 0  // 0=Giallo, 1=Verde, 2=Blu, 3=Rosso
```

### 2. Compila e carica

#### Con PlatformIO CLI:
```bash
# Compila
pio run

# Carica su dispositivo
pio run --target upload

# Monitor seriale
pio device monitor
```

#### Con VSCode + PlatformIO:
1. Apri il progetto in VSCode
2. Clicca su `PlatformIO: Upload` nella barra inferiore
3. Clicca su `PlatformIO: Serial Monitor` per vedere i log

### 3. Ripeti per tutti i 5 dispositivi

Ricorda di cambiare `IS_MASTER` e `SLAVE_ID` in `config.h` prima di ogni upload!

## 🎮 Come Funziona

### Fase 1: Connessione Slave

1. Accendi il **MASTER** → LED effetto arcobaleno (aspetta slave)
2. Accendi gli **SLAVE** → Ogni slave invia richiesta di connessione
3. Il master cicla i colori degli slave connessi
4. Quando tutti e 4 slave sono connessi → Master spegne LED (pronto)

### Fase 2: Gioco

1. **MASTER** preme il pulsante → Tutti i LED diventano VERDI 🟢
2. **SLAVE** preme il pulsante → Invia messaggio al master
3. Il **primo** slave che preme VINCE!
4. Tutti i LED si colorano del colore del vincitore

### Fase 3: Reset

- Dopo 5 secondi, il sistema torna in modalità READY
- Il master può premere di nuovo il pulsante per ripartire

## 🐛 Debugging

### Serial Monitor

Ogni dispositivo stampa log via seriale (115200 baud):

```
====================================
       PRENOTOMETRO v1.0
====================================
Device Mode: MASTER
====================================

ESP32 MAC Address: XX:XX:XX:XX:XX:XX
ESP-NOW initialized successfully
=== GameManager Begin ===
State change: 0 -> 1
```

### Comandi Utili

```bash
# Monitor con filtro
pio device monitor --filter colorize

# Monitor con baud rate specifico
pio device monitor --baud 115200

# Pulisci build
pio run --target clean
```

## 🔍 Troubleshooting

| Problema | Soluzione |
|----------|-----------|
| LED non si accendono | Controlla alimentazione 5V e pin GPIO18 |
| Pulsante non risponde | Verifica GPIO0 e connessione a GND |
| ESP-NOW non funziona | Controlla che tutti i dispositivi siano sulla stessa rete WiFi channel |
| Slave non si connette | Verifica `SLAVE_ID` univoco per ogni slave |
| Compilation error | Esegui `pio lib install` per installare dipendenze |

## 📝 Note

- **Distanza massima ESP-NOW**: ~100m in campo aperto, ~20-30m indoor
- **Latenza ESP-NOW**: < 10ms
- **Consumo LED**: ~60mA per LED (8 LED = ~480mA max a piena luminosità bianca)
- **Alimentazione consigliata**: USB 5V 2A

## 🎨 Personalizzazione

### Cambiare numero di LED

In [platformio.ini](platformio.ini):
```ini
build_flags =
    -D NUM_LEDS=16  # Cambia qui
```

### Cambiare colori

In [src/config.h](src/config.h):
```cpp
#define COLOR_YELLOW    0xFFFF00
#define COLOR_LIME      0x00FF00
#define COLOR_BLUE      0x0000FF
#define COLOR_RED       0xFF0000
```

### Cambiare timing

In [src/config.h](src/config.h):
```cpp
#define WINNER_DISPLAY_MS 5000  # Tempo display vincitore (ms)
#define CONNECTION_CYCLE_MS 500 # Velocità animazione connessione
```

## 🚀 Next Steps

- [ ] Aggiungere display OLED per mostrare stato
- [ ] Aggiungere buzzer per feedback audio
- [ ] Implementare modalità "best of 3"
- [ ] Aggiungere statistiche vincite
- [ ] Web server per configurazione WiFi
