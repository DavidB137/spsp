/**
 * @file espnow_types.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Types for ESPNOW classes
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <chrono>

#include "spsp/local_addr_mac.hpp"
#include "spsp/local_message.hpp"

namespace SPSP::LocalLayers::ESPNOW
{
    using LocalAddrT = SPSP::LocalAddrMAC;
    using LocalMessageT = SPSP::LocalMessage<SPSP::LocalAddrMAC>;

    /**
     * @brief ESP-NOW configuration
     *
     */
    struct Config
    {
        uint32_t ssid = 0x00000000;                    //!< Numeric SSID
        std::string password = std::string(32, 0x00);  //!< Password for packet payload encryption (must be 32 bytes long!)

        //! How long to wait for responses from bridge before switching to another WiFi channel
        std::chrono::milliseconds connectToBridgeChannelWaiting = std::chrono::milliseconds(100);

        //! Payload of PROBE_REQ message
        //! (gets reported to MQTT if `config.reporting.probePayload = true` on bridge)
        //! You probably want to put compile date or firmware version here.
        std::string probePayload = "";
    };
} // namespace SPSP::LocalLayers::ESPNOW
