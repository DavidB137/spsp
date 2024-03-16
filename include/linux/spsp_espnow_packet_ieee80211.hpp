/**
 * @file spsp_espnow_packet_ieee80211.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief ESP-NOW IEEE802.11 structures including radiotap
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <cstdint>

#include "spsp_espnow.hpp"
#include "spsp_espnow_packet.hpp"

namespace SPSP::LocalLayers::ESPNOW::IEEE80211
{
    static constexpr uint8_t FRAME_TYPE_ACTION = 0xD0;
    static constexpr uint8_t FRAME_TYPE_ACK = 0xD4;

    //! Length of radiotap fields
    using RadiotapTSFT = uint64_t;
    using RadiotapFlags = uint8_t;
    using RadiotapRate = uint8_t;
    struct RadiotapChannel { uint16_t freq; uint16_t flags; };
    using RadiotapFHSS = uint16_t;
    using RadiotapAntSignal = int8_t;
    using RadiotapExtention = uint32_t;

    /**
     * @brief Enumeration of parsed present field flags
     *
     * Not using `enum class` to remove casting.
     */
    enum RadiotapPresentFlags : uint32_t
    {
        RADIOTAP_PRESENT_TSFT       = (uint32_t) 1 << 0,
        RADIOTAP_PRESENT_FLAGS      = (uint32_t) 1 << 1,
        RADIOTAP_PRESENT_RATE       = (uint32_t) 1 << 2,
        RADIOTAP_PRESENT_CHANNEL    = (uint32_t) 1 << 3,
        RADIOTAP_PRESENT_FHSS       = (uint32_t) 1 << 4,
        RADIOTAP_PRESENT_ANT_SIGNAL = (uint32_t) 1 << 5,
        RADIOTAP_PRESENT_EXT        = (uint32_t) 1 << 31,
    };

    /**
     * @brief Parsed fields from radiotap header
     */
    struct RadiotapParsedFields
    {
        uint16_t len = 0;
        uint16_t freq = 0;
        int rssi = SIGNAL_MIN;
    };

    #pragma pack(push, 1)
    /**
     * @brief Vendor specific content of ESP-NOW
     *
     */
    struct VendorSpecificContent
    {
        uint8_t elementID = 221;
        uint8_t len;
        uint8_t oui[3] = {0x18, 0xFE, 0x34};
        uint8_t type = 0x04;
        uint8_t version = 0x01;
        uint8_t payload[];

        /**
         * @brief Calculates correct `len` based on actual payload length.
         *
         * @param payloadLen Length of payload
         */
        void setPayloadLen(uint8_t payloadLen) { len = payloadLen + 5; }

        /**
         * @brief Calculates actual payload length
         *
         * @return Payload length
         */
        uint8_t getPayloadLen() const { return len - 5; }
    };

    /**
     * @brief Action frame with vendor specific content
     *
     */
    struct ActionFrame
    {
        uint8_t type = FRAME_TYPE_ACTION;
        uint8_t flags = 0x00;
        uint16_t dur = 0x0000;
        uint8_t dst[6];
        uint8_t src[6];
        uint8_t bssid[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        uint16_t seq = 0x0000;  // Recalculated in hardware
        uint8_t cat = 0x7F;
        uint8_t oui[3] = {0x18, 0xFE, 0x34};
        uint8_t random[4];
        VendorSpecificContent content;
    };

    /**
     * @brief Generic frame
     *
     */
    struct GenericFrame
    {
        uint8_t type;
        uint8_t flags;
        uint16_t dur;
        uint8_t addr1[6];
    };

    /**
     * @brief Radiotap header
     *
     */
    struct Radiotap
    {
        uint8_t version = 0x00;
        uint8_t pad = 0x00;
        uint16_t len = sizeof(Radiotap);
        uint32_t present = 0x00000000;
    };

    /**
     * @brief Transmit radiotap header
     *
     * Doesn't include flag `FCS-at-end`.
     */
    struct TXRadiotap
    {
        Radiotap base = {
            .len = sizeof(TXRadiotap),
            .present = 0x00000004,
        };
        uint8_t datarate = 0x02;  // TODO: make configurable
    };

    /**
     * @brief Whole packet structure of ESP-NOW over radiotap
     *
     */
    struct ActionFrameWithRadiotap
    {
        TXRadiotap radiotap;
        ActionFrame action;
    };

    //! Maximum size of packet including payload
    //! Just exaggerated approximation
    static constexpr size_t MAX_PACKET_SIZE = 512;
} // namespace SPSP::LocalLayers::ESPNOW::IEEE80211
