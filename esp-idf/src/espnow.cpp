/**
 * @file espnow.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief ESP-NOW local layer for SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <chrono>
#include <cstring>
#include <thread>

#include "esp_mac.h"
#include "esp_now.h"
#include "esp_random.h"
#include "esp_wifi.h"

#include "spsp_chacha20.hpp"
#include "spsp_espnow.hpp"
#include "spsp_logger.hpp"
#include "spsp_node.hpp"
#include "spsp_wifi.hpp"

// Log tag
static const char* SPSP_LOG_TAG = "SPSP/Local/ESPNOW";

namespace SPSP::LocalLayers::ESPNOW
{
    // Instance pointer
    // ESP-NOW callbacks don't take `void*` context pointers, so we have to get
    // creative.
    static Layer* espnowInstance = nullptr;

    // Wrapper for C receive callback
    void _receiveCallback(const esp_now_recv_info_t* espnowInfo, const uint8_t* data, int dataLen)
    {
        espnowInstance->receiveCallback(espnowInfo->src_addr,
            const_cast<uint8_t*>(data), dataLen, espnowInfo->rx_ctrl->rssi);
    }

    // Wrapper for C send callback
    void _sendCallback(const uint8_t *dst, esp_now_send_status_t status)
    {
        espnowInstance->sendCallback(dst, status == ESP_NOW_SEND_SUCCESS);
    }

    Layer::Layer(uint32_t ssid, const std::string password)
        : m_ssid{ssid}, m_password{password}
    {
        // Store pointer to this instance
        // See above
        espnowInstance = this;

        // Initialize
        ESP_ERROR_CHECK(esp_now_init());

        // Register callbacks
        ESP_ERROR_CHECK(esp_now_register_recv_cb(_receiveCallback));
        ESP_ERROR_CHECK(esp_now_register_send_cb(_sendCallback));

        SPSP_LOGI("Initialized");
    }

    Layer::~Layer()
    {
        ESP_ERROR_CHECK(esp_now_deinit());
        SPSP_LOGI("Deinitialized");
    }

    bool Layer::send(const LocalMessage msg)
    {
        unsigned dataLen = sizeof(Packet) + msg.topic.length() + msg.payload.length();

        if (!this->validateMessage(msg)) return false;

        uint8_t* data = new uint8_t[dataLen];
        this->preparePacket(msg, data);

        SPSP_LOGD("Sending %u bytes to %s", dataLen, msg.addr.str.c_str());

        // Promise/mutex bucket
        auto bucketId = this->getBucketIdFromLocalAddr(msg.addr);

        // Lock both `m_mutex` and MAC's `m_sendingMutexes` entry without
        // deadlock
        std::lock(m_mutex, m_sendingMutexes[bucketId]);

        auto future = m_sendingPromises[bucketId].get_future();

        // Send
        this->sendRaw(msg.addr, data, dataLen);

        m_mutex.unlock();

        // Free memory
        delete[] data;
 
        // Wait for callback to finish
        bool delivered = future.get();

        // Reset promise
        m_sendingPromises[bucketId] = std::promise<bool>{};

        m_sendingMutexes[bucketId].unlock();

        SPSP_LOGD("Sent %u bytes to %s: %s", dataLen, msg.addr.str.c_str(),
                  delivered ? "success" : "fail");

        return delivered;
    }

    void Layer::sendRaw(const LocalAddr& dst, const uint8_t* data, size_t dataLen)
    {
        bool isBroadcast = dst == this->broadcastAddr();

        // Register peer
        if (!isBroadcast) {
            this->registerPeer(dst);
        }

        // Get MAC address
        esp_now_peer_info_t peerInfo = {};
        this->localAddrToMac(dst, peerInfo.peer_addr);

        // Send
        ESP_ERROR_CHECK(esp_now_send(peerInfo.peer_addr, data, dataLen));

        // Unregister peer
        if (!isBroadcast) {
            this->registerPeer(dst);
        }
    }

    bool Layer::validateMessage(const LocalMessage msg) const
    {
        unsigned dataLen = sizeof(Packet) + msg.topic.length() + msg.payload.length();

        if (dataLen > ESP_NOW_MAX_DATA_LEN) {
            SPSP_LOGE("Send fail: packet too big");
            return false;
        }

        return true;
    }

    void Layer::preparePacket(const LocalMessage msg, uint8_t* data) const
    {
        uint8_t topicLen = msg.topic.length();
        uint8_t payloadLen = msg.payload.length();
        unsigned dataLen = sizeof(Packet) + topicLen + payloadLen;

        Packet* p = reinterpret_cast<Packet*>(data);

        // Fill data
        p->header.ssid = m_ssid;
        p->header.version = PROTO_VERSION;
        p->payload.type = msg.type;
        p->payload.checksum = 0;
        p->payload.topicLen = topicLen;
        p->payload.payloadLen = payloadLen;

        // Copy topic and payload
        memcpy(p->payload.topicAndPayload, msg.topic.c_str(), topicLen);
        memcpy(p->payload.topicAndPayload + topicLen, msg.payload.c_str(),
               payloadLen);

        // Generate nonce
        esp_fill_random(&(p->header.nonce), NONCE_LEN);

        // Skip header
        data += sizeof(PacketHeader);
        dataLen -= sizeof(PacketHeader);

        // Checksum
        p->payload.checksum = this->checksumRaw(data, dataLen);

        // Encrypt
        this->encryptRaw(data, dataLen, p->header.nonce);
    }

    void Layer::registerPeer(const LocalAddr& addr)
    {
        // Get MAC address
        esp_now_peer_info_t peerInfo = {};
        this->localAddrToMac(addr, peerInfo.peer_addr);

        // Allow duplicate registrations
        auto ret = esp_now_add_peer(&peerInfo);
        if (ret != ESP_ERR_ESPNOW_EXIST) {
            ESP_ERROR_CHECK(ret);
        }
    }

    void Layer::unregisterPeer(const LocalAddr& addr)
    {
        // Get MAC address
        esp_now_peer_info_t peerInfo = {};
        this->localAddrToMac(addr, peerInfo.peer_addr);

        ESP_ERROR_CHECK(esp_now_del_peer(peerInfo.peer_addr));
    }

    void Layer::receiveCallback(const uint8_t* src, uint8_t* data, unsigned dataLen, int rssi) const
    {
        SPSP_LOGD("Receive: packet from %s", macTolocalAddr(src).str.c_str());

        // Check packet length
        if (dataLen < sizeof(Packet)) {
            SPSP_LOGD("Receive fail: packet too short");
            return;
        }

        // Treat as `Packet`
        Packet* p = reinterpret_cast<Packet*>(data);

        // Validate and decrypt
        if (!this->validatePacketHeader(p)) return;
        if (!this->decryptAndValidatePacketPayload(data, dataLen)) return;

        const char* topicAndPayload = reinterpret_cast<const char*>(p->payload.topicAndPayload);

        // Construct message
        LocalMessage msg;
        msg.type = p->payload.type;
        msg.addr = this->macTolocalAddr(src);
        msg.topic = std::string{topicAndPayload, p->payload.topicLen};
        msg.payload = std::string{topicAndPayload + p->payload.topicLen,
                              p->payload.payloadLen};

        // Send to the Node
        if (this->nodeConnected()) {
            this->getNode()->receiveLocal(msg, rssi);
        }
    }

    bool Layer::validatePacketHeader(const Packet* p) const
    {
        // Check SSID
        if (p->header.ssid != m_ssid) {
            SPSP_LOGD("Receive fail: different SSID");
            return false;
        }

        // Check version
        if (p->header.version != PROTO_VERSION) {
            SPSP_LOGD("Receive fail: different protocol version");
            return false;
        }

        return true;
    }

    bool Layer::decryptAndValidatePacketPayload(uint8_t* data, unsigned dataLen) const
    {
        // Treat as `Packet`
        Packet* p = reinterpret_cast<Packet*>(data);

        // Skip header
        data += sizeof(PacketHeader);
        dataLen -= sizeof(PacketHeader);

        // Decrypt
        this->encryptRaw(data, dataLen, p->header.nonce);

        // Check checksum
        uint8_t checksum = this->checksumRaw(data, dataLen, p->payload.checksum);

        if (checksum != p->payload.checksum) {
            SPSP_LOGD("Receive fail: invalid checksum");
            return false;
        }

        // Assert valid payload length
        if (sizeof(PacketPayload) + p->payload.topicLen + p->payload.payloadLen != dataLen) {
            SPSP_LOGD("Receive fail: invalid total length");
            return false;
        }

        return true;
    }

    void Layer::sendCallback(const uint8_t* dst, bool delivered)
    {
        // Promise/mutex bucket
        auto bucketId = this->getBucketIdFromLocalAddr(this->macTolocalAddr(dst));

        m_sendingPromises[bucketId].set_value(delivered);
    }

    bool Layer::connectToBridge(LocalAddr* br)
    {
        // Mutex
        const std::lock_guard lock(m_mutex);

        if (this->getNode()->isBridge()) {
            ESP_LOGE("Connect to bridge: this node is a bridge");
            return false;
        }

        WiFi& wifi = SPSP::WiFi::getInstance();

        // Get country restrictions
        wifi_country_t wifiCountry;
        ESP_ERROR_CHECK(esp_wifi_get_country(&wifiCountry));
        uint8_t lowCh = wifiCountry.schan;
        uint8_t highCh = lowCh + wifiCountry.nchan;

        SPSP_LOGI("Connect to bridge: country %s, channels %u - %u",
                 wifiCountry.cc, lowCh, highCh - 1);

        // Clear previous results
        m_bestBridgeSignal = SIGNAL_MIN;

        // Prepare message
        LocalMessage msg;
        msg.addr = this->broadcastAddr();
        msg.type = LocalMessageType::PROBE_REQ;

        // Convert to raw data
        size_t dataLen = sizeof(Packet);
        uint8_t* data = new uint8_t[dataLen];
        this->preparePacket(msg, data);

        // Probe all channels
        for (uint8_t ch = lowCh; ch < highCh; ch++) {
            wifi.setChannel(ch);
            this->sendRaw(msg.addr, data, dataLen);

            // Sleep
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // No response
        if (m_bestBridgeSignal == SIGNAL_MIN) {
            SPSP_LOGE("Connect to bridge: no response from bridge");
            return false;
        }

        // Register the bridge
        this->registerPeer(m_bestBridgeAddr);

        if (br != nullptr) {
            memcpy(br, &m_bestBridgeAddr, sizeof(m_bestBridgeAddr));
        }

        return true;
    }

    const LocalAddr Layer::macTolocalAddr(const uint8_t* mac)
    {
        LocalAddr la;

        // Internal representation
        la.addr = std::vector<uint8_t>(mac, mac + ESP_NOW_ETH_ALEN);

        // Printable string
        char macStr[16];
        sprintf(macStr, "%02x%02x%02x%02x%02x%02x", MAC2STR(mac));
        la.str = std::string(macStr);

        return la;
    }

    void Layer::localAddrToMac(const LocalAddr& la, uint8_t* mac)
    {
        for (unsigned i = 0; i < la.addr.size(); i++) {
            mac[i] = la.addr[i];
        }
    }

    void Layer::encryptRaw(uint8_t* data, unsigned dataLen, const uint8_t* nonce) const
    {
        Chacha20 chch{
            reinterpret_cast<const uint8_t*>(m_password.c_str()),
            nonce
        };

        chch.crypt(reinterpret_cast<uint8_t*>(data), dataLen);
    }

    uint8_t Layer::checksumRaw(uint8_t* data, unsigned dataLen, uint8_t existingChecksum)
    {
        uint8_t checksum = 0;

        for (unsigned i = 0; i < dataLen; i++) {
            checksum += data[i];
        }

        return checksum - existingChecksum;
    }

    uint8_t Layer::getBucketIdFromLocalAddr(const LocalAddr& addr) const
    {
        return std::hash<LocalAddr>{}(addr) % this->m_sendingPromises.size();
    }

    const LocalAddr Layer::broadcastAddr()
    {
        uint8_t bcst[ESP_NOW_ETH_ALEN];
        memset(bcst, 0xFF, ESP_NOW_ETH_ALEN);
        return Layer::macTolocalAddr(bcst);
    }
} // namespace SPSP::LocalLayers::ESPNOW
