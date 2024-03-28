/**
 * @file wifi_dummy.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Dummy WiFi adapter
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <string>
#include <unordered_set>

#include "spsp/wifi_espnow_if.hpp"
#include "spsp/wifi_types.hpp"

namespace SPSP::WiFi
{
    /**
     * @brief Dummy WiFi adapter
     *
     * It's not (currently) possible to change channels in monitor mode.
     */
    class Dummy : public IESPNOW
    {
    public:
        /**
         * @brief Gets current WiFi channel
         *
         * @return Channel number
         */
        uint8_t getChannel()
        {
            return 1;
        }

        /**
         * @brief Sets current channel
         *
         * Doesn't do anything.
         *
         * @param ch Channel
         */
        void setChannel(uint8_t ch) {}

        /**
         * @brief Get the Channel Restrictions object
         * 
         * @return const ChannelRestrictions 
         */
        const ChannelRestrictions getChannelRestrictions()
        {
            ChannelRestrictions rest = {};
            rest.low = 1;
            rest.high = 1;
            return rest;
        }
    };
} // namespace SPSP::WiFi
