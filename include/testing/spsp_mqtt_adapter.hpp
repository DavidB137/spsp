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
     * @brief Generic MQTT adapter for testing
     *
     */
    class Adapter : public IAdapter
    {
    public:
        /**
         * @brief Publishes message coming from node
         *
         * @param topic Topic
         * @param payload Payload (data)
         * @return true Delivery successful
         * @return false Delivery failed
         */
        virtual bool publish(const std::string& topic, const std::string& payload)
        {
            return true;
        }

        /**
         * @brief Subscribes to given topic
         *
         * @param topic Topic
         * @return true Subscribe successful
         * @return false Subscribe failed
         */
        virtual bool subscribe(const std::string& topic)
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
        virtual bool unsubscribe(const std::string& topic)
        {
            return true;
        }

        /**
         * @brief Sets callback for incoming subscription data
         *
         * @param cb Callback
         */
        virtual void setSubDataCb(AdapterSubDataCb cb) {}

        /**
         * @brief Sets connected callback
         *
         * Should be called on successful connection and reconnection.
         *
         * @param cb Callback
         */
        virtual void setConnectedCb(AdapterConnectedCb cb)
        {
            cb();
        }
    };
} // namespace SPSP::FarLayers::MQTT
