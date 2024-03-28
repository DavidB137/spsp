/**
 * @file mqtt_adapter_if.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Interface for platform-dependent MQTT adapter
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <functional>

#include "spsp/mqtt_types.hpp"

namespace SPSP::FarLayers::MQTT
{
    /**
     * @brief Adapter error
     *
     */
    class AdapterError : public SPSP::Exception
    {
        using SPSP::Exception::Exception;
    };

    // Callback types
    using AdapterConnectedCb = std::function<void()>;
    using AdapterSubDataCb = std::function<void(const std::string& topic,
                                                const std::string& payload)>;

    /**
     * @brief Interface for platform-dependent MQTT adapter
     *
     * Adapter doesn't have to check connection timeout.
     */
    class IAdapter
    {
    public:
        /**
         * @brief Publishes message coming from node
         *
         * This should not block (publish is very time sensitive).
         *
         * @param topic Topic
         * @param payload Payload (data)
         * @return true Delivery successful
         * @return false Delivery failed
         */
        virtual bool publish(const std::string& topic, const std::string& payload) = 0;

        /**
         * @brief Subscribes to given topic
         *
         * This should block (subscribe is usually not very time sensitive).
         *
         * @param topic Topic
         * @return true Subscribe successful
         * @return false Subscribe failed
         */
        virtual bool subscribe(const std::string& topic) = 0;

        /**
         * @brief Unsubscribes from given topic
         *
         * This should block (unsubscribe is usually not very time sensitive).
         *
         * @param topic Topic
         * @return true Unsubscribe successful
         * @return false Unsubscribe failed
         */
        virtual bool unsubscribe(const std::string& topic) = 0;

        /**
         * @brief Sets callback for incoming subscription data
         *
         * @param cb Callback
         */
        virtual void setSubDataCb(AdapterSubDataCb cb) = 0;

        /**
         * @brief Sets connected callback
         *
         * Should be called on successful connection and reconnection.
         *
         * @param cb Callback
         */
        virtual void setConnectedCb(AdapterConnectedCb cb) = 0;
    };
} // namespace SPSP::FarLayers::MQTT
