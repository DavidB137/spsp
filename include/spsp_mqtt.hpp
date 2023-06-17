/**
 * @file spsp_mqtt.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief MQTT far layer for SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include "spsp_interfaces.hpp"

namespace SPSP::FarLayers::MQTT
{
    /**
     * @brief MQTT far layer
     * 
     */
    class Layer : public SPSP::IFarLayer
    {
    protected:
        /**
         * @brief Publishes message coming from node
         * 
         * @param topic Topic
         * @param payload Payload (data)
         * @return true Delivery successful
         * @return false Delivery failed
         */
        bool publish(std::string topic, std::string payload);
    };
} // namespace SPSP::FarLayers::MQTT
