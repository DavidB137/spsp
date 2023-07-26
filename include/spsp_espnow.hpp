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
#include <future>
#include <mutex>
#include <string>

#include "spsp_layers.hpp"

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
     * @brief Bridge connection info
     * 
     * Needed for reconnection to the same bridge (i.e. after deep-sleep).
     */
    struct BridgeConnInfo
    {
        uint8_t addr[6];  //!< Address
        uint8_t ch;       //!< Wireless channel
    };

    // Assert sizes
    static_assert(sizeof(BridgeConnInfo) == 7);

    /**
     * @brief ESP-NOW local layer
     * 
     */
    class Layer : public SPSP::ILocalLayer
    {
    private:
        uint32_t m_ssid;                       //!< Numeric SSID
        std::string m_password;                //!< Password for packet payload encryption
        LocalAddr m_bestBridgeAddr = {};       //!< Address of bridge with the best signal
        int m_bestBridgeSignal = SIGNAL_MIN;   //!< Signal RSSI of bridge with the best signal
        uint8_t m_bestBridgeCh = 0;            //!< Channel of bridge with the best signal
        std::mutex m_mutex;                    //!< Mutex to prevent race conditions
        std::mutex m_bestBridgeMutex;          //!< Mutex for modifying m_bestBridge* attributes

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
         * @param ssid Service-set identifier
         * @param password Encryption password for communication (32 bytes)
         */
        Layer(uint32_t ssid, const std::string password);

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
        bool send(const LocalMessage msg);

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
         * Required size of memory under `rtndBr` and `connBr` is 7 bytes
         * (`sizeof(SPSP::LocalLayers::ESPNOW::BridgeConnInfo)`).
         * 
         * `rtndBr` and `connBr` may be the same pointers.
         * 
         * @param rtndBr Retained bridge peer info (for reconnection)
         * @param connBr Connected bridge peer info storage (if connection
         *               successful and `connBr` != nullptr)
         * @return true Connection successful
         * @return false Connection failed
         */
        bool connectToBridge(void* rtndBr = nullptr,
                             void* connBr = nullptr);

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

        /**
         * @brief Converts MAC address to `LocalAddr` instance
         * 
         * @param mac MAC address pointer
         * @return `LocalAddr` instance
         */
        static const LocalAddr macTolocalAddr(const uint8_t* mac);

        /**
         * @brief Converts `LocalAddr` instance to MAC address
         * 
         * @param la `LocalAddr` instance
         * @param mac MAC address pointer (modified in-place)
         * @return MAC address pointer
         */
        static void localAddrToMac(const LocalAddr& la, uint8_t* mac);

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
        void sendRaw(const LocalAddr& dst, const uint8_t* data, size_t dataLen);
        
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
        bool validateMessage(const LocalMessage msg) const;

        /**
         * @brief Prepares packet to be sent
         * 
         * Populates raw data with Packet object.
         * 
         * @param msg Message
         * @param data Raw memory for Packet
         */
        void preparePacket(const LocalMessage msg, uint8_t* data) const;

        /**
         * @brief Registers given peer
         * 
         * This is not multi-thread safe.
         * 
         * @param addr Address of the peer
         */
        void registerPeer(const LocalAddr& addr);

        /**
         * @brief Unregisters given peer
         * 
         * This is not multi-thread safe.
         * 
         * @param addr Address of the peer
         */
        void unregisterPeer(const LocalAddr& addr);

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
         * @brief Calculates bucket id from `LocalAddr` object
         * 
         * Used for `m_sendingPromises` and `m_sendingMutexes` arrays.
         * 
         * @param addr Address of the peer
         * @return Bucket id
         */
        uint8_t getBucketIdFromLocalAddr(const LocalAddr& addr) const;

        /**
         * @brief Returns broadcast address
         * 
         * @return Broadcast address
         */
        static const LocalAddr broadcastAddr();
    };
} // namespace SPSP::LocalLayers::ESPNOW
