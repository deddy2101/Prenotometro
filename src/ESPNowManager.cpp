#include "ESPNowManager.h"
#include "Logger.h"

// Inizializza callback statica
MessageCallback ESPNowManager::messageCallback = nullptr;

ESPNowManager::ESPNowManager() {
}

bool ESPNowManager::begin() {
    // Imposta WiFi in modalità Station
    WiFi.mode(WIFI_STA);

    Log.info("ESP32 MAC Address: %s", WiFi.macAddress().c_str());

    // Inizializza ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Log.error("ESP-NOW init failed");
        return false;
    }

    Log.info("ESP-NOW initialized successfully");

    // Registra callback
    esp_now_register_recv_cb(onDataRecv);
    esp_now_register_send_cb(onDataSent);

    // Aggiungi broadcast come peer (necessario per inviare in broadcast)
    esp_now_peer_info_t broadcastPeer = {};
    memcpy(broadcastPeer.peer_addr, broadcastAddress, 6);
    broadcastPeer.channel = ESP_NOW_CHANNEL;
    broadcastPeer.encrypt = false;
    if (esp_now_add_peer(&broadcastPeer) != ESP_OK) {
        Log.error("Failed to add broadcast peer");
        return false;
    }
    Log.info("Broadcast peer added");

    return true;
}

bool ESPNowManager::sendMessage(const Message& msg, const uint8_t* macAddr) {
    esp_err_t result;

    if (macAddr == nullptr) {
        // Invia in broadcast
        result = esp_now_send(broadcastAddress, (uint8_t*)&msg, sizeof(Message));
    } else {
        // Invia a specifico peer
        result = esp_now_send(macAddr, (uint8_t*)&msg, sizeof(Message));
    }

    if (result == ESP_OK) {
        Log.debug("Message sent successfully");
        return true;
    } else {
        Log.error("Send failed, error code: %d", result);
        return false;
    }
}

void ESPNowManager::setMessageCallback(MessageCallback callback) {
    messageCallback = callback;
}

bool ESPNowManager::addPeer(const uint8_t* macAddr) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, macAddr, 6);
    peerInfo.channel = ESP_NOW_CHANNEL;
    peerInfo.encrypt = false;

    esp_err_t result = esp_now_add_peer(&peerInfo);

    if (result == ESP_OK) {
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
        Log.info("Peer added: %s", macStr);
        return true;
    } else if (result == ESP_ERR_ESPNOW_EXIST) {
        Log.debug("Peer already exists");
        return true;
    } else {
        Log.error("Failed to add peer, error code: %d", result);
        return false;
    }
}

bool ESPNowManager::removePeer(const uint8_t* macAddr) {
    esp_err_t result = esp_now_del_peer(macAddr);
    return (result == ESP_OK);
}

void ESPNowManager::removeAllPeers() {
    esp_now_peer_num_t peerNum;
    esp_now_get_peer_num(&peerNum);

    Log.info("Removing %d peers", peerNum.total_num);

    // Note: in ESP-IDF v5.x potrebbe essere necessario usare un approccio diverso
    // per iterare sui peer. Questo è un placeholder.
}

void ESPNowManager::printMAC(const uint8_t* macAddr) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
    Serial.println(macStr);
}

void ESPNowManager::getMAC(uint8_t* macAddr) {
    WiFi.macAddress(macAddr);
}

// Callback ricezione dati
void ESPNowManager::onDataRecv(const uint8_t* macAddr, const uint8_t* data, int len) {
    if (len != sizeof(Message)) {
        Log.error("Received invalid message size: %d", len);
        return;
    }

    Message msg;
    memcpy(&msg, data, sizeof(Message));

    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             macAddr[0], macAddr[1], macAddr[2],
             macAddr[3], macAddr[4], macAddr[5]);

    Log.debug("RX from %s | Type: 0x%02X | SlaveID: %d", macStr, msg.type, msg.slaveId);

    // Chiama callback se registrata
    if (messageCallback != nullptr) {
        messageCallback(msg, macAddr);
    }
}

// Callback invio dati
void ESPNowManager::onDataSent(const uint8_t* macAddr, esp_now_send_status_t status) {
    Log.debug("TX Status: %s", status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}
