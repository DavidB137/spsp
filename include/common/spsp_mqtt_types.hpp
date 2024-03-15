/**
 * @file spsp_mqtt_types.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief MQTT types
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <chrono>
#include <cstdint>

#include "spsp_exception.hpp"

namespace SPSP::FarLayers::MQTT
{
    static constexpr const char* MQTT_CLIENT_ID_PREFIX = "spsp_";  //!< Default client ID prefix

    /**
     * @brief MQTT connection error
     *
     * Thrown when connection cannot be established within timeout.
     */
    class ConnectionError : public SPSP::Exception
    {
        using SPSP::Exception::Exception;
    };

    /**
     * @brief MQTT client configuration
     *
     */
    struct Config
    {
        struct Connection
        {
            std::string uri;        //!< Complete URI to connect to broker
                                    //!< e.g. mqtt://username:password@mqtt.eclipseprojects.io:1884/path

            /**
             * Verification TLS certificate (if TLS is used)
             *
             * On ESP platform, certificate content is expected.
             * On Linux platform, path to certificate file is expected.
             */
            std::string verifyCrt;

            uint32_t keepalive = 120;  //!< Keepalive interval in seconds (set to 0 to disable keepalive)
            int qos = 0;               //!< QoS for sent messages and subscriptions
            bool retain = false;       //!< Retain flag for sent messages

            std::chrono::milliseconds timeout = std::chrono::seconds(10);  //!< Connection timeout
        };

        struct Auth
        {
            std::string username;  //!< Username for connection (can also be set by URI)
            std::string password;  //!< Password for connection (can also be set by URI)

            /**
             * Client ID
             *
             * Default is: `spsp_xxx`, where `xxx` is MAC address.
             * Be aware that on Linux platform, MAC address is 00:00:00:00:00:00
             * by default and the real interface MAC address is set by
             * constructor of `SPSP::LocalLayers::ESPNOW::Adapter`.
             */
            std::string clientId;

            /**
             * Authentication TLS certificate (if needed).
             * 
             * If set, also `authKey` must be provided.
             *
             * On ESP platform, certificate content is expected.
             * On Linux platform, path to certificate file is expected.
             */
            std::string crt;

            /**
             * Private key for TLS authentication
             *
             * On ESP platform, certificate content is expected.
             * On Linux platform, path to certificate file is expected.
             */
            std::string crtKey;
        };

        struct LastWill
        {
            std::string topic;    //!< LWT topic
            std::string msg;      //!< LWT message
            int qos = 0;          //!< LWT QoS
            bool retain = false;  //!< LWT retain flag
        };

        /**
         * Topic prefix
         *
         * Topic format for publishing is: %TOPIC_PREFIX%/%ADDR%/%MSG_TOPIC%
         *
         * TODO: use C++20 format
         */
        std::string pubTopicPrefix = "spsp";

        Connection connection;
        Auth auth;
        LastWill lastWill;
    };
} // namespace SPSP::FarLayers::MQTT
