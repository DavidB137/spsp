/**
 * @file wifi.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief WiFi manager for ESP platform
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <string>

namespace SPSP
{
    const char* const WIFI_HOSTNAME_PREFIX = "spsp";

    /**
     * @brief WiFi manager for ESP platform (singleton)
     * 
     * Used by `SPSP::INode` classes.
     * 
     * In client modes, all roaming features and WPA3 are enabled.
     */
    class WiFi
    {
        std::string m_ssid;
        std::string m_password;

        WiFi() {}
    public:
        /**
         * @brief Singleton instantiator
         * 
         * @return This instance
         */
        static WiFi& instance()
        {
            static WiFi inst;
            return inst;
        }

        /**
         * @brief Initializes WiFi
         * 
         * If SSID is zero-length (or not given), no net interface is created
         * and WiFi is initalized in ESP-NOW-only mode.
         * 
         * @param ssid Service-set identifier
         * @param password Password for given SSID
         */
        void init(std::string ssid = "", std::string password = "");

        /**
         * @brief Deinitializes WiFi
         * 
         */
        void deinit();

        /**
         * @brief Gets current WiFi channel
         * 
         * @return Channel number
         */
        uint8_t getChannel();

        /**
         * @brief Sets current channel
         * 
         * @param ch Channel
         */
        void setChannel(uint8_t ch);
    };
} // namespace SPSP
