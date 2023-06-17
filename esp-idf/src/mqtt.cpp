/**
 * @file mqtt.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief MQTT far layer for SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "logger.hpp"
#include "spsp_mqtt.hpp"

namespace SPSP::FarLayers::MQTT
{
    bool Layer::publish(std::string topic, std::string payload)
    {
        return true;
    }
}
