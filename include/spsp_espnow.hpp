/**
 * @file spsp_espnow.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief ESP-NOW local layer for SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <string>

#include "spsp_interfaces.hpp"

namespace SPSP::LocalLayers::ESPNOW
{
    static const uint8_t PROTO_VERSION = 2;   //!< Current protocol version
    static const uint8_t SSID_LEN      = 8;   //!< SSID length in bytes
    static const uint8_t PASSWORD_LEN  = 32;  //!< Password length in bytes

    /**
     * @brief ESP-NOW local layer
     * 
     */
    class Layer : public SPSP::ILocalLayer
    {
    private:
        std::string ssid;
        std::string password;
        uint8_t datarate;

    public:
        /**
         * @brief Constructs a new ESP-NOW layer object
         * 
         * @param ssid Service-set identifier
         * @param password Encryption password for communication
         * @param datarate In case of ESP-IDF, use values of `wifi_phy_rate_t` enum.
         */
        Layer(const char ssid[SSID_LEN], const char password[PASSWORD_LEN], uint8_t datarate = 0)
            : ssid{ssid}, password{password}, datarate{datarate} {};

    protected:
        /**
         * @brief Sends the message to given node
         * 
         * @param msg Message
         */
        void send(Message msg);
    };
} // namespace SPSP::LocalLayers::ESPNOW
