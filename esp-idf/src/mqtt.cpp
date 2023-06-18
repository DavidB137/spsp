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
        // WiFi
        WiFi& wifi = WiFi::getInstance();
        wifi.init();

        SPSP_LOGI("Initialized");
    }

    Layer::~Layer()
    {
        // WiFi
        WiFi& wifi = WiFi::getInstance();
        wifi.deinit();

        SPSP_LOGI("Deinitialized");
    }

    bool Layer::publish(const std::string topic, const std::string payload)
    {
        return true;
    }

    bool Layer::subscribe(const std::string topic)
    {
        return true;
    }
}
