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
#include "spsp_version.hpp"
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
        // Create new thread for receive handler
        // Otherwise (apparently) creates deadlock, because receive callback
        // tries to send response, but ESP-NOW's internal mutex is still held
        // by this unfinished callback.
        std::thread t([espnowInfo, data, dataLen] {
            espnowInstance->receiveCallback(espnowInfo->src_addr,
                                            const_cast<uint8_t*>(data),
                                            dataLen,
                                            espnowInfo->rx_ctrl->rssi);
        });

        // Run independently
        t.detach();
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

        SPSP_LOGI("Protocol version: %d", PROTO_VERSION);
        SPSP_LOGI("Initialized");
    }

    Layer::~Layer()
    {
        ESP_ERROR_CHECK(esp_now_deinit());
        SPSP_LOGI("Deinitialized");
    }

    bool Layer::send(const LocalMessage msg)
    {
        SPSP_LOGD("Send: %s", msg.toString().c_str());

        LocalAddr dst = msg.addr;

        // Process empty destination address
        if (dst.empty()) {
            // Client: check discovered bridge
            // Bridge: `dst` shouldn't be ever empty

            if (!m_bestBridgeAddr.empty()) {
                dst = m_bestBridgeAddr;
                SPSP_LOGD("Send: rewriting destination MAC to %s", dst.str.c_str());
            } else {
                SPSP_LOGE("Send fail: destination address is empty and no bridge is connected");
                return false;
            }
        }

        unsigned dataLen = sizeof(Packet) + msg.topic.length() + msg.payload.length();

        if (!this->validateMessage(msg)) return false;

        uint8_t* data = new uint8_t[dataLen];
        this->preparePacket(msg, data);

        // Promise/mutex bucket
        auto bucketId = this->getBucketIdFromLocalAddr(dst);

        // Lock both `m_mutex` and MAC's `m_sendingMutexes` entry without
        // deadlock
        std::lock(m_mutex, m_sendingMutexes[bucketId]);

        auto future = m_sendingPromises[bucketId].get_future();

        // Send
        this->sendRaw(dst, data, dataLen);

        m_mutex.unlock();

        // Free memory
        delete[] data;

        SPSP_LOGD("Send: waiting for %s (bucket %d) callback",
                  dst.empty() ? "." : dst.str.c_str(), bucketId);
 
        // Wait for callback to finish
        // Note: this doesn't work for empty (NULL) destination MAC address
        bool delivered = future.get();

        // Reset promise
        m_sendingPromises[bucketId] = std::promise<bool>{};

        m_sendingMutexes[bucketId].unlock();

        SPSP_LOGD("Send: %u bytes to %s: %s", dataLen,
                  dst.empty() ? "." : dst.str.c_str(),
                  delivered ? "success" : "fail");

        return delivered;
    }

    void Layer::sendRaw(const LocalAddr& dst, const uint8_t* data, size_t dataLen)
    {
        if (dst.empty()) {
            // Don't register peer, just send data
            SPSP_LOGD("Send raw: %u bytes to %s", dataLen, ".");

            auto ret = esp_now_send(nullptr, data, dataLen);
            if (ret != ESP_ERR_ESPNOW_NOT_FOUND) ESP_ERROR_CHECK(ret);

            return;
        }

        // Register peer
        this->registerPeer(dst);

        // Get MAC address
        uint8_t peerMAC[ESP_NOW_ETH_ALEN];
        this->localAddrToMac(dst, peerMAC);

        // Send
        SPSP_LOGD("Send raw: %u bytes to %s", dataLen, dst.str.c_str());
        ESP_ERROR_CHECK(esp_now_send(peerMAC, data, dataLen));

        // Unregister peer
        this->unregisterPeer(dst);
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
        SPSP_LOGD("Register peer: %s", addr.str.c_str());

        // Get MAC address
        esp_now_peer_info_t peerInfo = {};
        this->localAddrToMac(addr, peerInfo.peer_addr);

        ESP_ERROR_CHECK(esp_now_add_peer(&peerInfo));
    }

    void Layer::unregisterPeer(const LocalAddr& addr)
    {
        SPSP_LOGD("Unregister peer: %s", addr.str.c_str());

        // Get MAC address
        esp_now_peer_info_t peerInfo = {};
        this->localAddrToMac(addr, peerInfo.peer_addr);

        ESP_ERROR_CHECK(esp_now_del_peer(peerInfo.peer_addr));
    }

    void Layer::receiveCallback(const uint8_t* src, uint8_t* data, unsigned dataLen, int rssi)
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
        LocalMessage msg = {};
        msg.type = p->payload.type;
        msg.addr = this->macTolocalAddr(src);
        msg.topic = std::string{topicAndPayload, p->payload.topicLen};
        msg.payload = std::string{topicAndPayload + p->payload.topicLen,
                              p->payload.payloadLen};

        // Process probe requests internally
        if (msg.type == LocalMessageType::PROBE_RES) {
            m_bestBridgeMutex.lock();

            SPSP_LOGI("Receive: probe response from %s (%d dBm)",
                      msg.addr.str.c_str(), rssi);

            // Store bridge with best signal
            if (m_bestBridgeSignal < rssi) {
                m_bestBridgeSignal = rssi;
                m_bestBridgeAddr = msg.addr;
            }

            m_bestBridgeMutex.unlock();
        }

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

        SPSP_LOGD("Send callback: %s (bucket %d): %s",
                  this->macTolocalAddr(dst).str.c_str(), bucketId,
                  delivered ? "delivered" : "not delivered");

        m_sendingPromises[bucketId].set_value(delivered);
    }

    bool Layer::connectToBridge(void* rtndBr, void* connBr)
    {
        // Mutex
        const std::lock_guard lock(m_mutex);

        if (!this->getNode()->isClient()) {
            SPSP_LOGE("Connect to bridge: this node is a bridge");
            return false;
        }

        auto* rtndBrStruct = reinterpret_cast<BridgeConnInfo*>(rtndBr);
        auto* connBrStruct = reinterpret_cast<BridgeConnInfo*>(connBr);

        WiFi& wifi = SPSP::WiFi::getInstance();

        if (rtndBrStruct != nullptr) {
            // Reconnect to retained bridge
            m_bestBridgeAddr = this->macTolocalAddr(rtndBrStruct->addr);
            wifi.setChannel(rtndBrStruct->ch);

            if (connBr != nullptr) {
                *connBrStruct = *rtndBrStruct;
            }

            SPSP_LOGI("Reconnected to bridge: %s",
                      m_bestBridgeAddr.str.c_str());

            return true;
        }

        SPSP_LOGD("Connect to bridge: connecting...");

        // Unregister old bridge (if present)
        if (!m_bestBridgeAddr.empty()) {
            this->unregisterPeer(m_bestBridgeAddr);
            SPSP_LOGD("Connect to bridge: unregistered old bridge");
        }

        // Get country restrictions
        wifi_country_t wifiCountry;
        ESP_ERROR_CHECK(esp_wifi_get_country(&wifiCountry));
        uint8_t lowCh = wifiCountry.schan;
        uint8_t highCh = lowCh + wifiCountry.nchan;

        SPSP_LOGI("Connect to bridge: country %c%c, channels %u - %u",
                 wifiCountry.cc[0], wifiCountry.cc[1], lowCh, highCh - 1);

        // Clear previous results
        m_bestBridgeSignal = SIGNAL_MIN;

        // Prepare message
        LocalMessage msg = {};
        msg.addr = this->broadcastAddr();
        msg.type = LocalMessageType::PROBE_REQ;
        msg.payload = SPSP::VERSION;

        // Convert to raw data
        size_t dataLen = sizeof(Packet);
        uint8_t data[ESP_NOW_MAX_DATA_LEN];
        this->preparePacket(msg, data);

        // Promise/mutex bucket
        auto bucketId = this->getBucketIdFromLocalAddr(msg.addr);

        // Probe all channels
        for (uint8_t ch = lowCh; ch < highCh; ch++) {
            auto future = m_sendingPromises[bucketId].get_future();

            wifi.setChannel(ch);
            this->sendRaw(msg.addr, data, dataLen);

            SPSP_LOGD("Connect to bridge: waiting for %s (bucket %d) callback",
                      msg.addr.str.c_str(), bucketId);

            // Wait for callback to finish
            future.get();

            // Reset promise
            m_sendingPromises[bucketId] = std::promise<bool>{};

            // Sleep
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // No response
        if (m_bestBridgeSignal == SIGNAL_MIN) {
            SPSP_LOGE("Connect to bridge: no response from bridge");
            return false;
        }

        SPSP_LOGI("Connected to bridge: %s (%d dBm)",
                  m_bestBridgeAddr.str.c_str(), m_bestBridgeSignal);

        if (connBrStruct != nullptr) {
            this->localAddrToMac(m_bestBridgeAddr, connBrStruct->addr);
            connBrStruct->ch = wifi.getChannel();
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
