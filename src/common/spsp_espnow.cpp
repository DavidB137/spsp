/**
 * @file spsp_espnow.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief ESP-NOW local layer for SPSP
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <cstring>
#include <functional>
#include <thread>

#include "spsp_espnow.hpp"
#include "spsp_logger.hpp"
#include "spsp_node.hpp"
#include "spsp_version.hpp"

// Log tag
static const char* SPSP_LOG_TAG = "SPSP/Local/ESPNOW";

namespace SPSP::LocalLayers::ESPNOW
{
    ESPNOW::ESPNOW(IAdapter& adapter, WiFi::IESPNOW& wifi, const Config& conf)
        : m_conf{conf}, m_wifi{wifi}, m_adapter{adapter}, m_serdes{m_conf}
    {
        using namespace std::placeholders;

        // Set callbacks
        m_adapter.setRecvCb(std::bind(&ESPNOW::recvCb, this, _1, _2, _3));
        m_adapter.setSendCb(std::bind(&ESPNOW::sendCb, this, _1, _2));

        SPSP_LOGI("Protocol version: %d", PROTO_VERSION);
        SPSP_LOGI("Initialized");
    }

    ESPNOW::~ESPNOW()
    {
        SPSP_LOGI("Deinitialized");
    }

    bool ESPNOW::send(const LocalMessageT& msg)
    {
        SPSP_LOGD("Send: %s", msg.toString().c_str());

        LocalAddrT dst = msg.addr;

        // Process empty destination address
        if (dst == LocalAddrT{}) {
            // Client: check discovered bridge
            // Bridge: `dst` shouldn't be ever empty

            if (!m_bestBridge.empty()) {
                dst = m_bestBridge.addr;
                SPSP_LOGD("Send: rewriting destination MAC to %s", dst.str.c_str());
            } else {
                SPSP_LOGE("Send fail: destination address is empty and no bridge is connected");
                return false;
            }
        }

        // Check length
        size_t dataLen = m_serdes.getPacketLength(msg);
        if (dataLen > MAX_PACKET_LENGTH) {
            SPSP_LOGE("Send fail: packet too big (%zu > %zu bytes)",
                      dataLen, MAX_PACKET_LENGTH);
            return false;
        }

        // Promise/mutex bucket
        auto bucketId = this->getBucketIdFromLocalAddr(dst);

        // Lock both `m_mutex` and MAC's `m_sendingMutexes` entry without
        // deadlock
        std::lock(m_mutex, m_sendingMutexes[bucketId]);

        auto future = m_sendingPromises[bucketId].get_future();

        // Create block, so `data` is destroyed immediatelly after sending
        {
            // Serialize
            std::string data;
            m_serdes.serialize(msg, data);

            // Send
            this->sendRaw(dst, data);
        }

        m_mutex.unlock();

        SPSP_LOGD("Send: waiting for %s (bucket %d) callback",
                  dst == LocalAddrT{} ? "." : dst.str.c_str(), bucketId);

        // Wait for callback to finish
        bool delivered = future.get();

        // Reset promise
        m_sendingPromises[bucketId] = std::promise<bool>{};

        m_sendingMutexes[bucketId].unlock();

        SPSP_LOGD("Send: %zu bytes to %s: %s", dataLen,
                  dst == LocalAddrT{} ? "." : dst.str.c_str(),
                  delivered ? "success" : "fail");

        return delivered;
    }

    bool ESPNOW::connectToBridge(BridgeConnInfoRTC* rtndBr,
                                 BridgeConnInfoRTC* connBr)
    {
        {   // Mutex
            const std::scoped_lock lock(m_mutex);

            if (rtndBr != nullptr) {
                // Reconnect to retained bridge
                m_bestBridge = *rtndBr;
                m_wifi.setChannel(m_bestBridge.ch);

                if (connBr != nullptr) {
                    *connBr = *rtndBr;
                }

                SPSP_LOGI("Reconnected to bridge: %s",
                          m_bestBridge.addr.str.c_str());

                return true;
            }

            SPSP_LOGD("Connect to bridge: connecting...");

            // Get country restrictions
            auto channelRestrictions = m_wifi.getChannelRestrictions();
            uint8_t lowCh = channelRestrictions.low;
            uint8_t highCh = channelRestrictions.high;

            SPSP_LOGI("Connect to bridge: channels %u - %u", lowCh, highCh);

            // Clear previous results
            m_bestBridge = {};

            // Prepare message
            LocalMessageT msg = {};
            msg.addr = LocalAddrT::broadcast();
            msg.type = LocalMessageType::PROBE_REQ;
            msg.payload = SPSP::VERSION;

            // Convert to raw data
            std::string data;
            m_serdes.serialize(msg, data);

            // Promise/mutex bucket
            auto bucketId = this->getBucketIdFromLocalAddr(msg.addr);

            // Probe all channels
            for (uint8_t ch = lowCh; ch <= highCh; ch++) {
                auto future = m_sendingPromises[bucketId].get_future();

                m_wifi.setChannel(ch);
                this->sendRaw(msg.addr, data);

                SPSP_LOGD("Connect to bridge: waiting for callback");

                // Wait for callback to finish
                future.get();

                // Reset promise
                m_sendingPromises[bucketId] = std::promise<bool>{};

                // Sleep
                std::this_thread::sleep_for(m_conf.connectToBridgeChannelWaiting);
            }

            // No response
            if (m_bestBridge.empty()) {
                SPSP_LOGE("Connect to bridge: no response from bridge");
                return false;
            }

            // New best bridge is available - switch to it's channel
            m_wifi.setChannel(m_bestBridge.ch);

            SPSP_LOGI("Connected to bridge: %s on channel %u (%d dBm)",
                      m_bestBridge.addr.str.c_str(), m_bestBridge.ch,
                      m_bestBridge.rssi);

            if (connBr != nullptr) {
                *connBr = m_bestBridge.toRTC();
            }
        }  // Mutex

        // Resubscribe to all topics
        if (this->nodeConnected()) {
            this->getNode()->resubscribeAll();
        }

        return true;
    }

    void ESPNOW::receive(const LocalMessageT& msg, int rssi)
    {
        // Process probe requests internally
        if (msg.type == LocalMessageType::PROBE_RES) {
            m_bestBridgeMutex.lock();

            SPSP_LOGI("Receive: probe response from %s (%d dBm)",
                      msg.addr.str.c_str(), rssi);

            // Store bridge with best signal
            if (m_bestBridge.rssi < rssi) {
                m_bestBridge.rssi = rssi;
                m_bestBridge.addr = msg.addr;
                m_bestBridge.ch = m_wifi.getChannel();
            }

            m_bestBridgeMutex.unlock();
        }

        // Send to the Node
        if (this->nodeConnected()) {
            this->getNode()->receiveLocal(msg, rssi);
        }
    }

    void ESPNOW::sendRaw(const LocalAddrT& dst, const std::string& data)
    {
        // Register peer
        m_adapter.addPeer(dst);

        // Send
        SPSP_LOGD("Send raw: %zu bytes to %s", data.length(), dst.str.c_str());
        m_adapter.send(dst, data);

        // Unregister peer
        m_adapter.removePeer(dst);
    }

    void ESPNOW::recvCb(const LocalAddrT src, std::string data, int rssi)
    {
        SPSP_LOGD("Receive: packet from %s", src.str.c_str());

        // Deserialize message
        LocalMessageT msg = {};
        if (!m_serdes.deserialize(src, data, msg)) {
            SPSP_LOGD("Receive: deserialization of packet from %s failed",
                      src.str.c_str());
            return;
        }

        this->receive(msg, rssi);
    }

    void ESPNOW::sendCb(const LocalAddrT dst, bool delivered)
    {
        // Promise/mutex bucket
        auto bucketId = this->getBucketIdFromLocalAddr(dst);

        SPSP_LOGD("Send callback: %s (bucket %d): %s",
                  LocalAddrT(dst).str.c_str(), bucketId,
                  delivered ? "delivered" : "not delivered");

        m_sendingPromises[bucketId].set_value(delivered);
    }

    uint8_t ESPNOW::getBucketIdFromLocalAddr(const LocalAddrT& addr) const
    {
        return std::hash<LocalAddrT>{}(addr) % this->m_sendingPromises.size();
    }

    BridgeConnInfoRTC ESPNOW::BridgeConnInfoInternal::toRTC()
    {
        BridgeConnInfoRTC brRTC = {};
        addr.toMAC(brRTC.addr);
        brRTC.ch = ch;
        return brRTC;
    }
} // namespace SPSP::LocalLayers::ESPNOW
