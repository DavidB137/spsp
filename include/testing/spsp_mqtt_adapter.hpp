/**
 * @file spsp_mqtt_adapter.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief MQTT adapter for testing
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include "spsp_mqtt_adapter_if.hpp"
#include "spsp_mqtt_types.hpp"

namespace SPSP::FarLayers::MQTT
{
    /**
     * @brief MQTT adapter for testing
     *
     */
    class Adapter : public IAdapter
    {
        Config m_conf;  //!< Configuration

    public:
        /**
         * @brief Constructs a new MQTT layer object
         *
         * Requires already initialized WiFi (with IP address).
         *
         * @param conf Configuration
         * @throw AdapterError when MQTT client can't be created and started
         */
        Adapter(const Config& conf) : m_conf{conf}, IAdapter{m_conf} {}

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
        bool publish(const std::string& topic, const std::string& payload)
        {
            return true;
        }

        /**
         * @brief Subscribes to given topic
         *
         * This blocks.
         *
         * @param topic Topic
         * @return true Subscribe successful
         * @return false Subscribe failed
         */
        bool subscribe(const std::string& topic)
        {
            return true;
        }

        /**
         * @brief Unsubscribes from given topic
         *
         * This blocks.
         *
         * @param topic Topic
         * @return true Unsubscribe successful
         * @return false Unsubscribe failed
         */
        bool unsubscribe(const std::string& topic)
        {
            return true;
        }

        /**
         * @brief Sets callback for incoming subscription data
         *
         * @param cb Callback
         */
        void setSubDataCb(AdapterSubDataCb cb) {}

        /**
         * @brief Sets connected callback
         *
         * Should be called on successful connection and reconnection.
         *
         * @param cb Callback
         */
        void setConnectedCb(AdapterConnectedCb cb) {}
    };
} // namespace SPSP::FarLayers::MQTT
