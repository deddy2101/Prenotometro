#include "GameManager.h"
#include "Logger.h"

// Flag pulsante definito in main.cpp, serve per pulirlo al game start
extern volatile bool buttonFlag;

GameManager::GameManager(LEDController& ledController, ESPNowManager& espNowManager, bool isMaster, uint8_t slaveId)
    : leds(ledController), espNow(espNowManager), isMaster(isMaster), slaveId(slaveId) {

    currentState = STATE_INIT;
    numConnected = 0;
    winnerSlaveId = 0xFF;
    gameStartTime = 0;
    isConnected = false;
    lastConnectRetry = 0;
    lastHeartbeatSent = 0;
    lastMasterMessage = 0;
    buttonPressed = false;
    lastButtonPress = 0;
    lastAnimationUpdate = 0;
    lastMasterHeartbeatSent = 0;

    for (uint8_t i = 0; i < MAX_SLAVES; i++) {
        connectedSlaves[i] = 0xFF;
        lastHeartbeatReceived[i] = 0;
    }
}

void GameManager::begin() {
    Log.info("=== GameManager Begin ===");
    Log.info("Mode: %s", isMaster ? "MASTER" : "SLAVE");

    if (!isMaster) {
        Log.info("Slave ID: %d", slaveId);
    }

    if (isMaster) {
        setState(STATE_WAITING_CONNECTIONS);
    } else {
        setState(STATE_WAITING_START);
        sendConnectRequest();
    }
}

void GameManager::update() {
    if (isMaster) {
        updateMaster();
    } else {
        updateSlave();
    }
}

void GameManager::setState(GameState newState) {
    if (currentState == newState) return;

    Log.info("State change: %d -> %d", currentState, newState);

    currentState = newState;
    lastAnimationUpdate = millis();
}

// ==================== MASTER LOGIC ====================

void GameManager::updateMaster() {
    // Controlla heartbeat in tutti gli stati (tranne WAITING_CONNECTIONS)
    if (currentState != STATE_WAITING_CONNECTIONS && numConnected > 0) {
        checkHeartbeats();
    }

    // Invia heartbeat agli slave durante il gioco
    if (currentState == STATE_GAME_RUNNING || currentState == STATE_READY) {
        unsigned long now = millis();
        if (now - lastMasterHeartbeatSent >= MASTER_HEARTBEAT_INTERVAL_MS) {
            lastMasterHeartbeatSent = now;
            sendMasterHeartbeat();
        }
    }

    switch (currentState) {
        case STATE_WAITING_CONNECTIONS:
            // Anima LED ciclando tra i colori degli slave connessi
            if (numConnected > 0) {
                uint32_t connectedColors[MAX_SLAVES];
                for (uint8_t i = 0; i < numConnected; i++) {
                    connectedColors[i] = SLAVE_COLORS[connectedSlaves[i]];
                }
                leds.cycleColors(connectedColors, numConnected, CONNECTION_CYCLE_MS);
            } else {
                // Nessuno connesso: effetto arcobaleno
                leds.rainbow(2000);
            }

            // Se tutti e 4 slave sono connessi, passa a READY
            if (numConnected >= MAX_SLAVES) {
                setState(STATE_READY);
                leds.setColor(COLOR_OFF);
                Log.info("All slaves connected! Press button to start game.");
            }
            break;

        case STATE_READY:
            // Aspetta pressione pulsante master per iniziare
            break;

        case STATE_GAME_RUNNING:
            // LED rosa durante il gioco
            leds.setColor(COLOR_PINK);
            break;

        case STATE_WINNER_ANNOUNCED:
            // Mostra colore vincitore (aspetta pressione pulsante master per ripartire)
            if (winnerSlaveId < MAX_SLAVES) {
                leds.pulse(SLAVE_COLORS[winnerSlaveId], 1000);
            }
            break;

        default:
            break;
    }
}

void GameManager::updateSlave() {
    unsigned long now = millis();

    // Controlla se il master è ancora vivo (WAITING_START e GAME_RUNNING)
    if (isConnected &&
        (currentState == STATE_WAITING_START || currentState == STATE_GAME_RUNNING) &&
        lastMasterMessage > 0 &&
        (now - lastMasterMessage > HEARTBEAT_TIMEOUT_MS)) {
        Log.warn("Master timeout! Reconnecting...");
        isConnected = false;
        lastMasterMessage = 0;
        lastConnectRetry = 0;  // Forza retry immediato
        setState(STATE_WAITING_START);
    }

    // Retry connessione se non connesso
    if (!isConnected && (now - lastConnectRetry >= CONNECT_RETRY_MS)) {
        lastConnectRetry = now;
        sendConnectRequest();
    }

    // Invia heartbeat periodico se connesso
    if (isConnected && (now - lastHeartbeatSent >= HEARTBEAT_INTERVAL_MS)) {
        lastHeartbeatSent = now;
        sendHeartbeat();
    }

    switch (currentState) {
        case STATE_WAITING_START:
            // Anima LED con il proprio colore
            if (isConnected) {
                leds.pulse(SLAVE_COLORS[slaveId], 1000);
            } else {
                // Non ancora connesso: arcobaleno
                leds.rainbow(1500);
            }
            break;

        case STATE_GAME_RUNNING:
            // LED rosa, aspetta pressione pulsante
            leds.setColor(COLOR_PINK);
            break;

        case STATE_WINNER_ANNOUNCED:
            // Mostra colore vincitore
            if (winnerSlaveId < MAX_SLAVES) {
                leds.setColor(SLAVE_COLORS[winnerSlaveId]);
            }
            break;

        default:
            break;
    }
}

void GameManager::handleMessage(const Message& msg, const uint8_t* macAddr) {
    switch (msg.type) {
        case MSG_CONNECT_REQUEST:
            if (isMaster) {
                handleConnectRequest(msg, macAddr);
            }
            break;

        case MSG_CONNECT_ACK:
            if (!isMaster) {
                Log.info("Connected to Master!");
                isConnected = true;
                lastMasterMessage = millis();
            }
            break;

        case MSG_START_GAME:
            if (!isMaster) {
                Log.info("Game started by Master!");
                lastMasterMessage = millis();
                buttonFlag = false;  // Scarta eventuali pressioni precedenti
                setState(STATE_GAME_RUNNING);
            }
            break;

        case MSG_BUTTON_PRESSED:
            if (isMaster) {
                handleButtonPressedFromSlave(msg);
            }
            break;

        case MSG_WINNER_ANNOUNCE:
            if (!isMaster) {
                Log.info("Winner: Slave %d", msg.slaveId);
                lastMasterMessage = millis();
                winnerSlaveId = msg.slaveId;
                setState(STATE_WINNER_ANNOUNCED);
            }
            break;

        case MSG_HEARTBEAT:
            if (isMaster && msg.slaveId < MAX_SLAVES) {
                lastHeartbeatReceived[msg.slaveId] = millis();
                Log.debug("Heartbeat from Slave %d", msg.slaveId);
            }
            break;

        case MSG_MASTER_HEARTBEAT:
            if (!isMaster) {
                lastMasterMessage = millis();
                Log.debug("Master heartbeat received");
            }
            break;

        case MSG_FALSE_START:
            if (isMaster) {
                // Ritrasmetti a tutti gli slave
                Log.warn("False start from Slave %d!", msg.slaveId);
                Message fsMsg;
                fsMsg.type = MSG_FALSE_START;
                fsMsg.slaveId = msg.slaveId;
                fsMsg.data = 0;
                fsMsg.timestamp = millis();
                espNow.sendMessage(fsMsg);
            }
            // Tutti (master e slave) fanno il lampeggio rosso
            falseStartFlash();
            break;

        default:
            Log.warn("Unknown message type: 0x%02X", msg.type);
            break;
    }
}

void GameManager::handleButtonPress() {
    unsigned long now = millis();

    // Debounce
    if (now - lastButtonPress < BUTTON_DEBOUNCE_MS) {
        return;
    }
    lastButtonPress = now;

    Log.debug("Button pressed!");

    if (isMaster) {
        // Master: gestisce pressione pulsante in base allo stato
        if (currentState == STATE_READY) {
            // Avvia il gioco
            startGame();
        } else if (currentState == STATE_WINNER_ANNOUNCED) {
            // Torna a READY per un nuovo round
            Log.info("Master reset - ready for new round");
            setState(STATE_READY);
            leds.setColor(COLOR_OFF);
        }
    } else {
        // Slave: invia messaggio al master se gioco in corso
        if (currentState == STATE_GAME_RUNNING) {
            sendButtonPressed();
        } else if (currentState == STATE_WAITING_START && isConnected) {
            // Falsa partenza! Notifica il master
            Log.warn("False start! Button pressed before game start.");
            Message fsMsg;
            fsMsg.type = MSG_FALSE_START;
            fsMsg.slaveId = slaveId;
            fsMsg.data = 0;
            fsMsg.timestamp = millis();
            espNow.sendMessage(fsMsg);
            falseStartFlash();
        }
    }
}

// ==================== MASTER METHODS ====================

void GameManager::handleConnectRequest(const Message& msg, const uint8_t* macAddr) {
    uint8_t slaveId = msg.slaveId;

    Log.info("Connect request from Slave %d", slaveId);

    if (!isSlaveConnected(slaveId)) {
        addConnectedSlave(slaveId, macAddr);
    }

    // Aggiorna heartbeat (anche se già connesso, per gestire riconnessioni)
    lastHeartbeatReceived[slaveId] = millis();

    // Invia sempre ACK (lo slave potrebbe non aver ricevuto il precedente)
    Message ackMsg;
    ackMsg.type = MSG_CONNECT_ACK;
    ackMsg.slaveId = slaveId;
    ackMsg.data = 0;
    ackMsg.timestamp = millis();

    espNow.addPeer(macAddr);
    espNow.sendMessage(ackMsg, macAddr);
}

void GameManager::handleButtonPressedFromSlave(const Message& msg) {
    if (currentState != STATE_GAME_RUNNING) {
        Log.warn("Button press ignored (game not running)");
        return;
    }

    // Primo slave a premere vince!
    uint8_t slaveId = msg.slaveId;
    Log.info("*** WINNER: Slave %d ***", slaveId);

    announceWinner(slaveId);
}

void GameManager::startGame() {
    Log.info("*** Starting game! ***");

    gameStartTime = millis();
    setState(STATE_GAME_RUNNING);

    // Invia messaggio START_GAME in broadcast
    Message msg;
    msg.type = MSG_START_GAME;
    msg.slaveId = 0xFF;
    msg.data = 0;
    msg.timestamp = millis();

    espNow.sendMessage(msg);
}

void GameManager::sendMasterHeartbeat() {
    Message msg;
    msg.type = MSG_MASTER_HEARTBEAT;
    msg.slaveId = 0xFF;
    msg.data = 0;
    msg.timestamp = millis();
    espNow.sendMessage(msg);
}

void GameManager::announceWinner(uint8_t slaveId) {
    winnerSlaveId = slaveId;
    setState(STATE_WINNER_ANNOUNCED);
    gameStartTime = millis();

    // Invia messaggio WINNER_ANNOUNCE in broadcast
    Message msg;
    msg.type = MSG_WINNER_ANNOUNCE;
    msg.slaveId = slaveId;
    msg.data = 0;
    msg.timestamp = millis();

    espNow.sendMessage(msg);
}

// ==================== SLAVE METHODS ====================

void GameManager::sendConnectRequest() {
    Log.info("Sending connect request to Master...");

    Message msg;
    msg.type = MSG_CONNECT_REQUEST;
    msg.slaveId = slaveId;
    msg.data = 0;
    msg.timestamp = millis();

    espNow.sendMessage(msg);
}

void GameManager::sendButtonPressed() {
    Log.info("Sending button press to Master!");

    Message msg;
    msg.type = MSG_BUTTON_PRESSED;
    msg.slaveId = slaveId;
    msg.data = 0;
    msg.timestamp = millis();

    espNow.sendMessage(msg);

    // Cambio stato locale (ottimistico)
    setState(STATE_WINNER_ANNOUNCED);
    winnerSlaveId = slaveId;
}

void GameManager::sendHeartbeat() {
    Message msg;
    msg.type = MSG_HEARTBEAT;
    msg.slaveId = slaveId;
    msg.data = 0;
    msg.timestamp = millis();

    espNow.sendMessage(msg);
}

void GameManager::checkHeartbeats() {
    unsigned long now = millis();

    for (uint8_t i = 0; i < numConnected; i++) {
        uint8_t id = connectedSlaves[i];
        if (id < MAX_SLAVES && lastHeartbeatReceived[id] > 0 &&
            (now - lastHeartbeatReceived[id] > HEARTBEAT_TIMEOUT_MS)) {

            Log.warn("Slave %d disconnected! (no heartbeat for %ds)",
                     id, HEARTBEAT_TIMEOUT_MS / 1000);
            removeConnectedSlave(id);

            // Annulla round e torna ad aspettare connessioni
            setState(STATE_WAITING_CONNECTIONS);
            leds.setColor(COLOR_OFF);
            Log.info("Round cancelled. Waiting for all slaves to reconnect...");
            return;  // Esci, l'array è stato modificato
        }
    }
}

void GameManager::removeConnectedSlave(uint8_t id) {
    for (uint8_t i = 0; i < numConnected; i++) {
        if (connectedSlaves[i] == id) {
            // Shifta gli elementi rimanenti
            for (uint8_t j = i; j < numConnected - 1; j++) {
                connectedSlaves[j] = connectedSlaves[j + 1];
                memcpy(slaveMacs[j], slaveMacs[j + 1], 6);
            }
            connectedSlaves[numConnected - 1] = 0xFF;
            numConnected--;
            lastHeartbeatReceived[id] = 0;
            Log.info("Slave %d removed. Total: %d/%d", id, numConnected, MAX_SLAVES);
            return;
        }
    }
}

void GameManager::falseStartFlash() {
    // 3 lampeggi rossi veloci
    for (int i = 0; i < 3; i++) {
        leds.setColor(COLOR_RED);
        delay(200);
        leds.setColor(COLOR_OFF);
        delay(200);
    }
    buttonFlag = false;  // Pulisci eventuali pressioni durante il flash
}

// ==================== UTILITY ====================

bool GameManager::isSlaveConnected(uint8_t id) {
    for (uint8_t i = 0; i < numConnected; i++) {
        if (connectedSlaves[i] == id) {
            return true;
        }
    }
    return false;
}

void GameManager::addConnectedSlave(uint8_t id, const uint8_t* macAddr) {
    if (numConnected >= MAX_SLAVES) {
        Log.error("Max slaves reached");
        return;
    }

    connectedSlaves[numConnected] = id;
    memcpy(slaveMacs[numConnected], macAddr, 6);
    numConnected++;

    Log.info("Slave %d connected. Total: %d/%d", id, numConnected, MAX_SLAVES);
}
