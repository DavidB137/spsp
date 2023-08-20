/**
 * @file spsp_wifi_country_restrictions.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Country restrictions struct
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <cstdint>

namespace SPSP::WiFi
{
    /**
     * @brief WiFi country restrictions structure
     *
     */
    struct CountryRestrictions
    {
        const char cc[3];  //!< Country code (two letters and null byte)
        uint8_t lowCh;     //!< Lowest usable channel
        uint8_t highCh;    //!< Highest usable channel
    };
} // namespace SPSP::WiFi

