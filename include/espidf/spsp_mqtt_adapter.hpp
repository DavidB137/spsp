/**
 * @file spsp_mqtt_adapter.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief MQTT adapter for ESP platform
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include "mqtt_client.h"

#include "spsp_mqtt_adapter_if.hpp"
#include "spsp_mqtt_types.hpp"

namespace SPSP::FarLayers::MQTT
{
    /**
     * @brief MQTT adapter for ESP platform
     *
     * Doesn't check if connection was successfully established within time limit.
     *
     * Only one MQTT instance can use this at a time and
     * there may be many `Adapter` instances at a time.
     */
    class Adapter : public IAdapter
    {
        esp_mqtt_client_handle_t m_mqtt;             //!< MQTT client handle
        Config m_conf;                               //!< Configuration
        AdapterSubDataCb m_subDataCb = nullptr;      //!< Subscription data callback
        AdapterConnectedCb m_connectedCb = nullptr;  //!< Connected callback

    public:
        /**
         * @brief Constructs a new MQTT layer object
         *
         * Requires already initialized WiFi (with IP address).
         *
         * @param conf Configuration
         * @throw AdapterError when MQTT client can't be created and started
         */
        Adapter(const Config& conf);

        /**
         * @brief Destroys MQTT layer object
         *
         */
        ~Adapter();

        /**
         * @brief Publishes message coming from node
         *
         * This doesn't block.
         *
         * @param topic Topic
         * @param payload Payload (data)
         * @return true Delivery successful
         * @return false Delivery failed
         */
        bool publish(const std::string& topic, const std::string& payload);

        /**
         * @brief Subscribes to given topic
         *
         * This blocks.
         *
         * @param topic Topic
         * @return true Subscribe successful
         * @return false Subscribe failed
         */
        bool subscribe(const std::string& topic);

        /**
         * @brief Unsubscribes from given topic
         *
         * This blocks.
         *
         * @param topic Topic
         * @return true Unsubscribe successful
         * @return false Unsubscribe failed
         */
        bool unsubscribe(const std::string& topic);

        /**
         * @brief Sets callback for incoming subscription data
         *
         * @param cb Callback
         */
        void setSubDataCb(AdapterSubDataCb cb);

        /**
         * @brief Gets callback for incoming subscription data
         *
         * @return Callback
         */
        AdapterSubDataCb getSubDataCb() const;

        /**
         * @brief Sets connected callback
         *
         * Should be called on successful connection and reconnection.
         *
         * @param cb Callback
         */
        void setConnectedCb(AdapterConnectedCb cb);

        /**
         * @brief Gets connected callback
         *
         * @return Callback
         */
        AdapterConnectedCb getConnectedCb() const;

    protected:
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
