/**
 * @file spsp_wifi_espnow.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief WiFi interface for ESP-NOW
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <cstdint>

#include "spsp_wifi_country_restrictions.hpp"

namespace SPSP::WiFi
{
    /**
     * @brief Requirements of ESP-NOW from WiFi instance
     *
     * Should be implemented by platform-specific WiFi adapter.
     */
    class IESPNOW
    {
    public:
        /**
         * @brief Gets current WiFi channel
         *
         * @return Current WiFi channel
         */
        virtual uint8_t getChannel() = 0;

        /**
         * @brief Sets current WiFi channel
         *
         * May do nothing, but in this case return value of
         * `getCountryRestrictions()` must hold `lowCh = highCh`.
         *
         * @param ch New current WiFi channel
         */
        virtual void setChannel(uint8_t ch) = 0;

        /**
         * @brief Gets currently set country restrictions of WiFi adapter
         *
         * @return Country restrictions
         */
        virtual const CountryRestrictions getCountryRestrictions() = 0;
    };
} // namespace SPSP::WiFi

