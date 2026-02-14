# 🎮 Prenotometro

Sistema di prenotazione basato su ESP32-S2 Mini con comunicazione ESP-NOW.

## 📝 Descrizione

Gioco con 5 dispositivi ESP32-S2 Mini:
- **1 Master** (controller di gioco)
- **4 Slave** (pulsanti colorati: 🟡 Giallo, 🟢 Verde, 🔵 Blu, 🔴 Rosso)

### Funzionamento

1. **Fase iniziale**: il master aspetta che tutti gli slave si connettano, ciclando sui colori dei dispositivi connessi
2. **Start Game**: il master avvia il gioco, tutti i LED diventano verdi 🟢
3. **Prenotazione**: il primo slave che preme il pulsante vince
4. **Vittoria**: tutti i dispositivi si illuminano del colore del vincitore
5. Si attende un nuovo "start game"

## 🛠️ Hardware

- **Microcontroller**: ESP32-S2 Mini (x5)
- **LED**: WS2812B (8 LED per dispositivo)
- **Pulsante**: Push button

### Pin utilizzati

- `GPIO18`: LED WS2812B (Data In)
- `GPIO0`: Pulsante (con pull-up interno)

## 📦 Software

- **PlatformIO** + Arduino Framework
- **ESP-NOW** per comunicazione wireless
- **FastLED** / **Adafruit NeoPixel** per controllo LED

### Architettura

```
src/
├── LEDController    # Gestione LED WS2812B
├── ESPNowManager    # Comunicazione ESP-NOW
├── GameManager      # Logica del gioco
└── main.cpp         # Sketch principale
```

## 🚀 Build & Upload

```bash
# Compila
pio run

# Upload su Master
pio run --target upload

# Monitor seriale
pio device monitor
```

## 📡 Configurazione Master/Slave

Modificare in `config.h`:
```cpp
#define IS_MASTER true  // false per slave
```

## 🎨 Colori Slave

| Slave | Colore | Hex     |
|-------|--------|---------|
| 1     | 🟡 Giallo | #FFFF00 |
| 2     | 🟢 Verde  | #00FF00 |
| 3     | 🔵 Blu    | #0000FF |
| 4     | 🔴 Rosso  | #FF0000 |
