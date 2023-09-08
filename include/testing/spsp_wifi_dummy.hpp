/**
 * @file spsp_wifi_dummy.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Dummy WiFi adapter for testing
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <string>
#include <unordered_set>

#include "spsp_wifi_espnow_if.hpp"
#include "spsp_wifi_types.hpp"

namespace SPSP::WiFi
{
    /**
     * @brief Dummy WiFi adapter for testing
     *
     * Allowed channels are 1 - 5.
     */
    class Dummy : public IESPNOW
    {
        uint8_t m_channel = 1;

    public:
        /**
         * @brief Gets current WiFi channel
         *
         * @return Channel number
         */
        uint8_t getChannel()
        {
            return m_channel;
        }

        /**
         * @brief Sets current channel
         *
         * @param ch Channel
         */
        void setChannel(uint8_t ch)
        {
            m_channel = ch;
        }

        /**
         * @brief Get the Channel Restrictions object
         * 
         * @return const ChannelRestrictions 
         */
        const ChannelRestrictions getChannelRestrictions()
        {
            ChannelRestrictions rest = {};
            rest.low = 1;
            rest.high = 5;
            return rest;
        }
    };
} // namespace SPSP::WiFi
