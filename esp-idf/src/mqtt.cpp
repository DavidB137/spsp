/**
 * @file mqtt.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief MQTT far layer for SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "spsp_logger.hpp"
#include "spsp_mqtt.hpp"
#include "spsp_wifi.hpp"

// Log tag
static const char* SPSP_LOG_TAG = "SPSP/Far/MQTT";

namespace SPSP::FarLayers::MQTT
{
    Layer::Layer()
    {
        SPSP_LOGI("Initialized");
    }

    Layer::~Layer()
    {
        SPSP_LOGI("Deinitialized");
    }

    bool Layer::publish(const std::string src, const std::string topic,
                        const std::string payload)
    {
        SPSP_LOGD("Publish: payload %s to topic %s from %s",
                  payload.c_str(), topic.c_str(), src.c_str());
        return true;
    }

    bool Layer::subscribe(const std::string topic)
    {
        SPSP_LOGD("Subscribe: %s", topic.c_str());
        return true;
    }

    bool Layer::unsubscribe(const std::string topic)
    {
        SPSP_LOGD("Unsubscribe: %s", topic.c_str());
        return true;
    }
}
