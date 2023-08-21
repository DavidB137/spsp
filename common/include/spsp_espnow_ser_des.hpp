/**
 * @file spsp_espnow_ser_des.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief ESP-NOW packet serializer and deserializer
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include "spsp_espnow_types.hpp"
#include "spsp_espnow_packet.hpp"
#include "spsp_random.hpp"

namespace SPSP::LocalLayers::ESPNOW
{
    /**
     * @brief Serializer and deserializer of ESP-NOW packets
     *
     * Converts raw data to `LocalMessage` instances and back.
     */
    class SerDes
    {
        const Config& m_conf;  //!< Configuration reference
        Random m_rand;         //!< Random generator

    public:
        /**
         * @brief Construct a new serializer/deserializer
         *
         * @param conf Configuration
         */
        SerDes(const Config& conf) noexcept;

        /**
         * @brief Serializes local message to raw data
         *
         * Total message length is not checked!
         *
         * @param msg Message input
         * @param data Raw data output
         */
        void serialize(const LocalMessageT& msg, std::string& data) const noexcept;

        /**
         * @brief Deserializes raw data to local message
         *
         * @param src Source address
         * @param data Raw data input
         * @param msg Message output
         * @return true Deserialization successful
         * @return false Deserialization failed
         */
        bool deserialize(const LocalAddrT& src, std::string& data,
                         LocalMessageT& msg) const noexcept;

        /**
         * @brief Calculates total packet length
         *
         * Useful for checking whether packet size is within limits.
         *
         * @param msg Message
         * @return Length
         */
        static size_t getPacketLength(const LocalMessageT& msg) noexcept;

    protected:
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
        static uint8_t checksumRaw(uint8_t* data, size_t dataLen,
                                   uint8_t existingChecksum = 0) noexcept;

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
        void encryptRaw(uint8_t* data, size_t dataLen, const uint8_t* nonce) const noexcept;

        /**
         * @brief Validates packet's header
         *
         * @param p Packet
         * @return true Header is valid
         * @return false Header is invalid
         */
        bool validatePacketHeader(const Packet* p) const;

        /**
         * @brief Decrypts and validates packet's payload
         *
         * @param data Raw packet data
         * @param dataLen Length of data
         * @return true Payload is valid
         * @return false Payload is invalid
         */
        bool decryptAndValidatePacketPayload(uint8_t* data, size_t dataLen) const;
    };
} // namespace SPSP::LocalLayers::ESPNOW
