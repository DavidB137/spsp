/**
 * @file spsp_espnow.hpp
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

#include "spsp_layers.hpp"
#include "spsp_local_addr_mac.hpp"

namespace SPSP::LocalLayers::ESPNOW
{
    static const uint8_t PROTO_VERSION = 1;        //!< Current protocol version
    static const uint8_t PASSWORD_LEN  = 32;       //!< Password length in bytes
    static const uint8_t NONCE_LEN     = 8;        //!< Length of encryption nonce
    static const int SIGNAL_MIN        = INT_MIN;  //!< Worst signal value

    /**
     * @brief Maximum number of simultaneous peers
     *
     * Peers are added and removed during each message sending, so this really
     * only limits number of concurrent "deliveries". Concurrent "deliveries"
     * over this limit will have to wait in queue.
     */
    static const uint8_t MAX_PEER_NUM = 15;

    #pragma pack(push, 1)
    /**
     * @brief ESP-NOW packet header
     *
     * Constains SSID and encryption nonce.
     */
    struct PacketHeader
    {
        uint32_t ssid;              //!< Service set identifier
        uint8_t nonce[NONCE_LEN];   //!< Encryption nonce
        uint8_t version;            //!< Current protocol version
    };

    struct PacketPayload
    {
        LocalMessageType type;      //!< Message type
        uint8_t _reserved[3];       //!< Reserved for future use
        uint8_t checksum;           //!< Simple checksum of `PacketPayload` to validate decrypted packet
        uint8_t topicLen;           //!< Length of topic
        uint8_t payloadLen;         //!< Length of payload (data)
        uint8_t topicAndPayload[];  //!< Topic and payload as string (not null terminated)
    };

    struct Packet
    {
        PacketHeader header;
        PacketPayload payload;
    };
    #pragma pack(pop)

    // Assert sizes
    static_assert(sizeof(PacketHeader) == 13);
    static_assert(sizeof(PacketPayload) == 7);
    static_assert(sizeof(Packet) == 20);

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
     * @brief ESP-NOW configuration
     *
     */
    struct Config
    {
        uint32_t ssid;         //!< Numeric SSID
        std::string password;  //!< Password for packet payload encryption
    };

    /**
     * @brief ESP-NOW local layer
     *
     */
    class Layer : public ILocalLayer<LocalMessage<LocalAddrMAC>>
    {
    public:
        using LocalAddrT = typename SPSP::LocalAddrMAC;
        using LocalMessageT = typename SPSP::LocalMessage<SPSP::LocalAddrMAC>;

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
            bool empty() { return addr.empty(); }

            /**
             * @brief Convert internal info to RTC version
             *
             * @return RTC version
             */
            BridgeConnInfoRTC toRTC();
        };

        uint32_t m_ssid;                           //!< Numeric SSID
        std::string m_password;                    //!< Password for packet payload encryption
        BridgeConnInfoInternal m_bestBridge = {};  //!< Bridge with best signal
        std::mutex m_mutex;                        //!< Mutex to prevent race conditions
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
         * Only one instance may exist at the same time.
         *
         * @param config Configuration
         */
        Layer(const Config config);

        /**
         * @brief Destroys ESP-NOW layer object
         *
         */
        ~Layer();

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
        bool send(const LocalMessageT msg);

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
         * @param rtndBr Retained bridge peer info (for reconnection)
         * @param connBr Connected bridge peer info storage (if connection
         *               successful and `connBr` != nullptr)
         * @return true Connection successful
         * @return false Connection failed
         */
        bool connectToBridge(BridgeConnInfoRTC* rtndBr = nullptr,
                             BridgeConnInfoRTC* connBr = nullptr);

        /**
         * @brief Receive callback for underlaying ESP-NOW C functions.
         *
         * Used indirectly.
         *
         * @param src Source address
         * @param data Raw data
         * @param dataLen Length of data
         * @param rssi Received signal strength indicator (in dBm)
         */
        void receiveCallback(const uint8_t* src, uint8_t* data, unsigned dataLen, int rssi);

        /**
         * @brief Send callback for underlaying ESP-NOW C functions.
         *
         * Used indirectly.
         *
         * @param dst Destination address
         * @param delivered Whether packet has been delivered successfully
         */
        void sendCallback(const uint8_t* dst, bool delivered);

    protected:
        /**
         * @brief Sends raw packet to the underlaying library
         *
         * Also registers and unregisters the peer temporarily.
         *
         * This is not multi-thread safe.
         *
         * @param dst Destination address
         * @param data Raw data
         * @param dataLen Length of data
         */
        void sendRaw(const LocalAddrT& dst, const uint8_t* data, size_t dataLen);

        /**
         * @brief Encryption provider for raw data (bytes)
         *
         * Wraps ChaCha20 encryption.
         * This function is used for both encryption and decryption.
         * Data are encrypted/decrypted in-place.
         *
         * @param data Data to encrypt/decrypt
         * @param dataLen Length of data
         * @param nonce Encryption nonce
         */
        void encryptRaw(uint8_t* data, unsigned dataLen, const uint8_t* nonce) const;

        /**
         * @brief Validates packet's header
         *
         * @param p Packet
         * @return true Packet header is valid
         * @return false Packet header is invalid
         */
        bool validatePacketHeader(const Packet* p) const;

        /**
         * @brief Decrypts and validates packet's payload
         *
         * @param data Raw packet data
         * @param dataLen Length of data
         * @return true Packet header is valid
         * @return false Packet header is invalid
         */
        bool decryptAndValidatePacketPayload(uint8_t* data, unsigned dataLen) const;

        /**
         * @brief Validates whether message can be sent
         *
         * @param msg Message
         * @return true Message can be sent
         * @return false Message can't be sent
         */
        bool validateMessage(const LocalMessageT msg) const;

        /**
         * @brief Prepares packet to be sent
         *
         * Populates raw data with Packet object.
         *
         * @param msg Message
         * @param data Raw memory for Packet
         */
        void preparePacket(const LocalMessageT msg, uint8_t* data) const;

        /**
         * @brief Registers given peer
         *
         * This is not multi-thread safe.
         *
         * @param addr Address of the peer
         */
        void registerPeer(const LocalAddrT& addr);

        /**
         * @brief Unregisters given peer
         *
         * This is not multi-thread safe.
         *
         * @param addr Address of the peer
         */
        void unregisterPeer(const LocalAddrT& addr);

        /**
         * @brief Checksums the given raw data (bytes)
         *
         * Optionally subtracts existing checksum.
         *
         * @param data Data to encrypt/decrypt
         * @param dataLen Length of data
         * @param existingChecksum Existing checksum (to subtract)
         * @return Correct checksum
         */
        static uint8_t checksumRaw(uint8_t* data, unsigned dataLen, uint8_t existingChecksum = 0);

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
