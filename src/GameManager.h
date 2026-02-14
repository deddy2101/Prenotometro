#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include "LEDController.h"
#include "ESPNowManager.h"

class GameManager {
public:
    GameManager(LEDController& ledController, ESPNowManager& espNowManager, bool isMaster, uint8_t slaveId = 0);

    void begin();
    void update();

    // Gestione stato
    GameState getState() const { return currentState; }
    void setState(GameState newState);

    // Handler messaggi ESP-NOW
    void handleMessage(const Message& msg, const uint8_t* macAddr);

    // Gestione pulsante
    void handleButtonPress();

private:
    LEDController& leds;
    ESPNowManager& espNow;

    bool isMaster;
    uint8_t slaveId;
    GameState currentState;

    // Master specific
    uint8_t connectedSlaves[MAX_SLAVES];
    uint8_t slaveMacs[MAX_SLAVES][6];
    uint8_t numConnected;
    uint8_t winnerSlaveId;
    unsigned long gameStartTime;

    // Slave specific
    bool isConnected;

    // Button debounce
    bool buttonPressed;
    unsigned long lastButtonPress;

    // Timing
    unsigned long lastAnimationUpdate;

    // Metodi privati Master
    void updateMaster();
    void handleConnectRequest(const Message& msg, const uint8_t* macAddr);
    void handleButtonPressedFromSlave(const Message& msg);
    void startGame();
    void announceWinner(uint8_t slaveId);

    // Metodi privati Slave
    void updateSlave();
    void sendConnectRequest();
    void sendButtonPressed();

    // Utility
    bool isSlaveConnected(uint8_t id);
    void addConnectedSlave(uint8_t id, const uint8_t* macAddr);
};

#endif // GAME_MANAGER_H
