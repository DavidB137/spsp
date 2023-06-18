/**
 * @file espnow.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief ESP-NOW local layer for SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "spsp_logger.hpp"
#include "spsp_espnow.hpp"
#include "spsp_wifi.hpp"

// Log tag
static const char* SPSP_LOG_TAG = "SPSP/Local/ESPNOW";

namespace SPSP::LocalLayers::ESPNOW
{
    Layer::Layer(const char ssid[SSID_LEN], const char password[PASSWORD_LEN], uint8_t datarate)
        : m_ssid{ssid}, m_password{password}, m_datarate{datarate}
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

    bool Layer::send(const Message msg)
    {
        return true;
    }
} // namespace SPSP::LocalLayers::ESPNOW
