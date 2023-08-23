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
            std::string verifyCrt;  //!< Verification TLS certificate (if TLS is used)
            int keepalive = 120;    //!< Keepalive interval in seconds (set to 0 to disable keepalive)
            int qos = 0;            //!< QoS for sent messages
            bool retain = false;    //!< Retain flag for sent messages

            std::chrono::milliseconds timeout = std::chrono::seconds(10);  //!< Connection timeout
        };

        struct Auth
        {
            std::string username;  //!< Username for connection (can also be set by URI)
            std::string password;  //!< Password for connection (can also be set by URI)
            std::string clientId;  //!< Client ID (default is: `spsp_xxx`, where `xxx` is MAC address)
            std::string crt;       //!< Authentication TLS certificate (if needed).
                                   //!< If set, also `authKey` must be provided.
            std::string crtKey;    //!< Private key for TLS authentication
        };

        struct LastWill
        {
            std::string topic;    //!< LWT topic
            std::string msg;      //!< LWT message
            int qos = 0;          //!< LWT QoS
            bool retain = false;  //!< LWT retain flag
        };

        std::string pubTopicPrefix = "spsp";  //!< Topic format for publishing is:
                                              //!< %TOPIC_PREFIX%/%ADDR%/%MSG_TOPIC%
                                              //!< TODO: use C++20 format
        Connection connection;
        Auth auth;
        LastWill lastWill;
    };
} // namespace SPSP::FarLayers::MQTT
