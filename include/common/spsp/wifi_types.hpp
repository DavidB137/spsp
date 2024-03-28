/**
 * @file wifi_types.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Common WiFi types (and some constants)
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <cstdint>

#include "spsp/exception.hpp"

namespace SPSP::WiFi
{
    static constexpr int TX_POWER_DEFAULT = INT_MIN;  //!< Default TX power

    /**
     * @brief WiFi connection error
     *
     */
    class ConnectionError : public SPSP::Exception
    {
        using SPSP::Exception::Exception;
    };

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

