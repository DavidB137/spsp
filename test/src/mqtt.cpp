/**
 * @file mqtt.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Testing MQTT far layer
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <chrono>
#include <thread>

#include "spsp_logger.hpp"
#include "spsp_mqtt.hpp"

// Log tag
static const char* SPSP_LOG_TAG = "SPSP/Far/MQTT";

namespace SPSP::FarLayers::MQTT
{
    Layer::Layer(const ClientConfig config)
        : m_conf{config}
    {
    }

    Layer::~Layer()
    {
    }

    void Layer::connected()
    {
    }

    bool Layer::publish(const std::string& src, const std::string& topic,
                        const std::string& payload)
    {
        return true;
    }

    bool Layer::subscribe(const std::string& topic)
    {
        return true;
    }

    bool Layer::unsubscribe(const std::string& topic)
    {
        return true;
    }
}
