/**
 * @file spsp_wifi_channel_restrictions.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief WiFi channel restrictions struct
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <cstdint>

namespace SPSP::WiFi
{
    /**
     * @brief WiFi channel restrictions structure
     *
     */
    struct ChannelRestrictions
    {
        uint8_t low;   //!< Lowest usable channel
        uint8_t high;  //!< Highest usable channel
    };
} // namespace SPSP::WiFi

