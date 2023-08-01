/**
 * @file spsp_mqtt.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief MQTT far layer for SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <exception>
#include <future>

#include "spsp_layers.hpp"
#include "spsp_node.hpp"

namespace SPSP::FarLayers::MQTT
{
    const char* const MQTT_CLIENT_ID_PREFIX = "spsp_";           //!< Default client ID prefix
    const auto MQTT_CONNECT_TIMEOUT = std::chrono::seconds(10);  //!< Timeout for connecting to broker

    /**
     * @brief MQTT connection error
     * 
     * Thrown when `MQTT_CONNECT_TIMEOUT` expires before successful connection.
     */
    class MQTTConnectionError : public std::exception {};

    /**
     * @brief MQTT client configuration
     * 
     */
    struct ClientConfig
    {
        struct Connection
        {
            std::string uri;        //!< Complete URI to connect to broker
                                    //!< e.g. mqtt://username:password@mqtt.eclipseprojects.io:1884/path
            std::string verifyCrt;  //!< Verification TLS certificate (if TLS is used)
            int keepalive = 120;    //!< Keepalive interval in seconds (set to 0 to disable keepalive)
            int qos = 0;            //!< QoS for sent messages
            bool retain = false;    //!< Retain flag for sent messages
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

    /**
     * @brief MQTT far layer
     * 
     */
    class Layer : public IFarLayer
    {
        void* m_mqtt;                            //!< MQTT client handle
        std::string m_pubTopicPrefix;            //!< Topic prefix (see `ClientConfig`)
        int m_qos = 0;                           //!< QoS for sent messages
        bool m_retain = false;                   //!< Retain flag for sent messages
        bool m_initializing = true;              //!< Whether we are currently in initializing phase
        std::promise<void> m_connectingPromise;  //!< Promise to block until successful connection is made

    public:
        using ConfigT = typename SPSP::FarLayers::MQTT::ClientConfig;

        /**
         * @brief Constructs a new MQTT layer object
         * 
         * Requires already initialized WiFi (with IP address).
         * 
         * Block until connection is successfully made.
         * May throw `MQTTConnectionError`.
         * 
         * @param config Configuration
         */
        Layer(const ConfigT config);

        /**
         * @brief Destroys MQTT layer object
         * 
         */
        ~Layer();

        /**
         * @brief Signalizes successful initial connection to broker
         * 
         */
        void connected();

    protected:
        /**
         * @brief Publishes message coming from node
         * 
         * @param src Source address
         * @param topic Topic
         * @param payload Payload (data)
         * @return true Delivery successful
         * @return false Delivery failed
         */
        bool publish(const std::string src, const std::string topic,
                     const std::string payload);

        /**
         * @brief Subscribes to given topic
         * 
         * Should be used by `INode` only!
         * 
         * @param topic Topic
         * @return true Subscribe successful
         * @return false Subscribe failed
         */
        bool subscribe(const std::string topic);

        /**
         * @brief Unsubscribes from given topic
         * 
         * Should be used by `INode` only!
         * 
         * @param topic Topic
         * @return true Unsubscribe successful
         * @return false Unsubscribe failed
         */
        bool unsubscribe(const std::string topic);

        /**
         * @brief Helper to convert `std::string` to C string or `nullptr`
         * 
         * @param str String
         * @return If string is empty, `nullptr`, otherwise C string. 
         */
        inline static const char* stringToCOrNull(const std::string& str)
        {
            return str.empty() ? nullptr : str.c_str();
        }
    };
} // namespace SPSP::FarLayers::MQTT
