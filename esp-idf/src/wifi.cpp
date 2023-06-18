/**
 * @file wifi.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief WiFi manager for ESP platform
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <cstring>
#include <string>

#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "logger.hpp"
#include "spsp_wifi.hpp"

// Log tag
static const char* SPSP_LOG_TAG = "SPSP/WiFi";

namespace SPSP
{
    void WiFi::init(std::string ssid, std::string password)
    {
        // Don't do anything if already initialized
        if (this->initialized) return;

        // Store given parameters
        this->m_ssid = ssid;
        this->m_password = password;

        bool ssidNotEmpty = ssid.length() > 0;

        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());

        // Create net interface
        if (ssidNotEmpty) {
            esp_netif_t *netif = esp_netif_create_default_wifi_sta();

            // Set hostname
            std::string hostname = WIFI_HOSTNAME_PREFIX;
            uint8_t mac[8];
            char macStr[16];
            ESP_ERROR_CHECK(esp_efuse_mac_get_default(mac));
            sprintf(macStr, "%02x%02x%02x%02x%02x%02x", MAC2STR(mac));
            hostname += macStr;
            esp_netif_set_hostname(netif, hostname.c_str());
        }

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        // Events
        // TODO

        // WiFi config
        if (ssidNotEmpty) {
            wifi_config_t wifi_config = {};
            strcpy((char*) wifi_config.sta.ssid, ssid.c_str());
            strcpy((char*) wifi_config.sta.password, password.c_str());

            // Enable roaming features
            wifi_config.sta.rm_enabled = true;
            wifi_config.sta.btm_enabled = true;
            wifi_config.sta.mbo_enabled = true;
            wifi_config.sta.ft_enabled = true;

            // Enable security features
            wifi_config.sta.owe_enabled = true;
            wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        }

        // Set station mode and start WiFi
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());

        // No power saving for bridge (IMPORTANT!)
        if (ssidNotEmpty) {
            esp_wifi_set_ps(WIFI_PS_NONE);
        }

        this->initialized = true;

        SPSP_LOGI("Initialized");
    }

    void WiFi::deinit()
    {
        // Don't do anything if already deinitialized
        if (!this->initialized) return;

        ESP_ERROR_CHECK(esp_wifi_stop());
        ESP_ERROR_CHECK(esp_wifi_deinit());
        ESP_ERROR_CHECK(esp_event_loop_delete_default());
        //ESP_ERROR_CHECK(esp_netif_deinit());

        SPSP_LOGI("Deinitialized");
    }

    uint8_t WiFi::getChannel()
    {
        uint8_t ch;
        wifi_second_chan_t sec;
        ESP_ERROR_CHECK(esp_wifi_get_channel(&ch, &sec));
        return ch;
    }

    void WiFi::setChannel(uint8_t ch)
    {
        ESP_ERROR_CHECK(esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE));
        SPSP_LOGI("Set channel %d", ch);
    }
} // namespace SPSP
