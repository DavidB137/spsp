/**
 * @file espnow.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief ESP-NOW local layer for SPSP
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <array>
#include <climits>
#include <future>
#include <mutex>
#include <string>

#include "spsp/espnow_adapter_if.hpp"
#include "spsp/espnow_packet.hpp"
#include "spsp/espnow_ser_des.hpp"
#include "spsp/espnow_types.hpp"
#include "spsp/layers.hpp"
#include "spsp/local_addr_mac.hpp"
#include "spsp/wifi_espnow_if.hpp"

namespace SPSP::LocalLayers::ESPNOW
{
    static constexpr int SIGNAL_MIN = INT_MIN;  //!< Worst signal value

    /**
     * @brief Maximum number of simultaneous peers
     *
     * Peers are added and removed during each message sending, so this really
     * only limits number of concurrent "deliveries". Concurrent "deliveries"
     * over this limit will have to wait in queue.
     */
    static constexpr uint8_t MAX_PEER_NUM = 15;

    /**
     * @brief RTC memory enabled bridge connection info
     *
     * Needed for reconnection to the same bridge (i.e. after deep-sleep).
     */
    struct BridgeConnInfoRTC
    {
        uint8_t addr[6];  //!< Address
        uint8_t ch;       //!< Wireless channel
    };

    /**
     * @brief ESP-NOW local layer
     *
     */
    class ESPNOW : public ILocalLayer<LocalMessageT>
    {
    public:
        using LocalAddrT = SPSP::LocalLayers::ESPNOW::LocalAddrT;
        using LocalMessageT = SPSP::LocalLayers::ESPNOW::LocalMessageT;
    
    protected:
        /**
         * @brief Internal bridge connection info
         *
         */
        struct BridgeConnInfoInternal
        {
            LocalAddrT addr = {};   //!< Address
            int rssi = SIGNAL_MIN;  //!< Signal RSSI
            uint8_t ch = 0;         //!< Wireless channel

            /**
             * @brief Construct a new empty object
             *
             */
            BridgeConnInfoInternal() {}

            /**
             * @brief Construct a new object from retained RTC version
             *
             * @param brRTC RTC version
             */
            BridgeConnInfoInternal(BridgeConnInfoRTC brRTC)
                : addr{brRTC.addr}, ch{brRTC.ch} {};

            /**
             * @brief Whether bridge info doesn't contain any meaningful bridge
             *
             * @return true Bridge info is empty
             * @return false Bridge info is not empty
             */
            bool empty() { return addr == LocalAddrT{}; }

            /**
             * @brief Convert internal info to RTC version
             *
             * @return RTC version
             */
            BridgeConnInfoRTC toRTC();
        };

        std::mutex m_mutex;                        //!< Mutex to prevent race conditions
        const Config m_conf;                       //!< Configuration
        WiFi::IESPNOW& m_wifi;                     //!< WiFi instance
        IAdapter& m_adapter;                       //!< Low level ESP-NOW adapter
        SerDes m_serdes;                           //!< Packet serializer/deserializer
        BridgeConnInfoInternal m_bestBridge = {};  //!< Bridge with best signal
        std::mutex m_bestBridgeMutex;              //!< Mutex for modifying m_bestBridge* attributes

        /**
         * @brief Container for promises of being-sent messages
         *
         * `send()` blocks until status of delivery is available (and no other
         * message is being sent to the same node).
         * Assign each peer a "bucket". When sending a message to other peer
         * from the same bucket, wait for it to finish. Additional support for
         * this mechanism is provided by `m_sendingMutexes`.
         */
        std::array<std::promise<bool>, MAX_PEER_NUM> m_sendingPromises;

        /**
         * @brief Container for mutex of being-sent messages
         *
         * We need only one thread waiting for message status per bucket in
         * `m_sendingPromises`.
         * Efectively allows maximum of `MAX_PEER_NUM` messages being sent
         * at the same time.
         */
        std::array<std::mutex, MAX_PEER_NUM> m_sendingMutexes;
    
    public:
        /**
         * @brief Constructs a new ESP-NOW layer object
         *
         * Requires already initialized WiFi.
         *
         * @param adapter ESP-NOW low-level adapter
         * @param wifi WiFi instance
         * @param conf Configuration
         */
        ESPNOW(IAdapter& adapter, WiFi::IESPNOW& wifi, const Config& conf);

        /**
         * @brief Destroys ESP-NOW layer object
         *
         */
        ~ESPNOW();

        /**
         * @brief Sends the message to given node
         *
         * In the message, empty address means send to the bridge peer.
         *
         * Note: this will block until delivery status is confirmed!
         *
         * @param msg Message
         * @return true Delivery successful
         * @return false Delivery failed
         */
        bool send(const LocalMessageT& msg);

        /**
         * @brief Connects to the bridge
         *
         * If `rtndBr` is not `nullptr`, uses it's address, returns
         * `true` immediately, does no scan whatsoever and `connBr = rtndBr`.
         * Otherwise probes all wireless channels (available in the currently
         * configured country) and selects bridge with the best signal.
         *
         * You can use `retainedBridge` and `connectedBridge` parameters to
         * implement custom logic around deep-sleep reconnection without scans.
         *
         * `rtndBr` and `connBr` may be the same pointers.
         *
         * Automatically resubscribes to all topics.
         *
         * @param rtndBr Retained bridge peer info (for reconnection)
         * @param connBr Connected bridge peer info storage (if connection
         *               successful and `connBr` != nullptr)
         * @return true Connection successful
         * @return false Connection failed
         */
        bool connectToBridge(BridgeConnInfoRTC* rtndBr = nullptr,
                             BridgeConnInfoRTC* connBr = nullptr);

    protected:
        /**
         * @brief Receive message handler
         *
         * Separate from `recvCb` to allow simpler testing.
         *
         * @param msg Message
         * @param rssi Received signal strength indicator (in dBm)
         */
        void receive(const LocalMessageT& msg, int rssi);

        /**
         * @brief Sends raw packet to the underlaying library
         *
         * Also registers and unregisters the peer temporarily.
         *
         * This is not multi-thread safe.
         *
         * @param dst Destination address
         * @param data Raw data
         */
        void sendRaw(const LocalAddrT& dst, const std::string& data);

        /**
         * @brief Receive callback for underlaying ESP-NOW adapter
         *
         * @param src Source address
         * @param data Raw data
         * @param rssi Received signal strength indicator (in dBm)
         */
        void recvCb(const LocalAddrT src, std::string data, int rssi);

        /**
         * @brief Send callback for underlaying ESP-NOW adapter
         *
         * @param dst Destination address
         * @param delivered Whether packet has been delivered successfully
         */
        void sendCb(const LocalAddrT dst, bool delivered);

        /**
         * @brief Calculates bucket id from `LocalAddrT` object
         *
         * Used for `m_sendingPromises` and `m_sendingMutexes` arrays.
         *
         * @param addr Address of the peer
         * @return Bucket id
         */
        uint8_t getBucketIdFromLocalAddr(const LocalAddrT& addr) const;
    };
} // namespace SPSP::LocalLayers::ESPNOW
