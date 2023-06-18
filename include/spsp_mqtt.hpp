/**
 * @file spsp_mqtt.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief MQTT far layer for SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include "spsp_layers.hpp"

namespace SPSP::FarLayers::MQTT
{
    /**
     * @brief MQTT far layer
     * 
     */
    class Layer : public SPSP::IFarLayer
    {
    public:
        /**
         * @brief Constructs a new MQTT layer object
         * 
         * Also initializes WiFi (if not already initialized).
         */
        Layer();

        /**
         * @brief Destroys MQTT layer object
         * 
         */
        ~Layer();

    protected:
        /**
         * @brief Publishes message coming from node
         * 
         * @param topic Topic
         * @param payload Payload (data)
         * @return true Delivery successful
         * @return false Delivery failed
         */
        bool publish(const std::string topic, const std::string payload);

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
    };
} // namespace SPSP::FarLayers::MQTT
