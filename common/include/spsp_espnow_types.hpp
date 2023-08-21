/**
 * @file spsp_espnow_types.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Types for ESPNOW classes
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include "spsp_local_addr_mac.hpp"
#include "spsp_local_message.hpp"

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
        uint32_t ssid;         //!< Numeric SSID
        std::string password;  //!< Password for packet payload encryption
    };
} // namespace SPSP::LocalLayers::ESPNOW
