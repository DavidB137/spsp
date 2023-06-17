/**
 * @file spsp_mqtt.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief MQTT far layer for SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef SPSP_MQTT_HPP
#define SPSP_MQTT_HPP

#include "spsp.hpp"

namespace SPSP::FarLayers::MQTT
{
    /**
     * @brief MQTT far layer
     * 
     */
    class Layer : public SPSP::FarLayer
    {
    };
} // namespace SPSP::FarLayers::MQTT

#endif
