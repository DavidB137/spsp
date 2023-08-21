/**
 * @file spsp_espnow_packet.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief ESP-NOW packet structures
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <cstdint>

#include "spsp_local_message.hpp"

namespace SPSP::LocalLayers::ESPNOW
{
    static constexpr uint8_t PROTO_VERSION     = 1;    //!< Current protocol version
    static constexpr uint8_t PASSWORD_LEN      = 32;   //!< Password length in bytes
    static constexpr uint8_t NONCE_LEN         = 8;    //!< Length of encryption nonce
    static constexpr size_t  MAX_PACKET_LENGTH = 250;  //!< Maximum total packet length

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
} // namespace SPSP::LocalLayers::ESPNOW
