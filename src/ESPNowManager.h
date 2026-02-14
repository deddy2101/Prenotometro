#ifndef ESPNOW_MANAGER_H
#define ESPNOW_MANAGER_H

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "config.h"

// Callback per ricezione messaggi
typedef void (*MessageCallback)(const Message& msg, const uint8_t* macAddr);

class ESPNowManager {
public:
    ESPNowManager();

    bool begin();
    bool sendMessage(const Message& msg, const uint8_t* macAddr = nullptr);
    void setMessageCallback(MessageCallback callback);

    // Gestione peer
    bool addPeer(const uint8_t* macAddr);
    bool removePeer(const uint8_t* macAddr);
    void removeAllPeers();

    // Utility
    void printMAC(const uint8_t* macAddr);
    void getMAC(uint8_t* macAddr);

private:
    static MessageCallback messageCallback;

    // Callback ESP-NOW statiche
    static void onDataRecv(const uint8_t* macAddr, const uint8_t* data, int len);
    static void onDataSent(const uint8_t* macAddr, esp_now_send_status_t status);
};

#endif // ESPNOW_MANAGER_H
