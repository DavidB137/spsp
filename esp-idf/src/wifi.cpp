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
#include "nvs_flash.h"

#include "spsp_logger.hpp"
#include "spsp_wifi.hpp"

// Log tag
static const char* SPSP_LOG_TAG = "SPSP/WiFi";

namespace SPSP
{
    void WiFi::init(const std::string ssid, const std::string password)
    {
        // Mutex
        const std::lock_guard<std::mutex> lock(m_mutex);

        // Don't do anything if already initialized
        if (this->m_initialized) return;

        // Store given parameters
        this->m_ssid = ssid;
        this->m_password = password;

        // Create event loop
        ESP_ERROR_CHECK(esp_event_loop_create_default());

        // Initialize NVS and network
        this->initNVS();
        this->initNetIf();

        // Register handlers
        this->registerEventHandlers();

        // Init configuration
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

        // Use current config settings
        this->initWiFiConfig();

        // Start WiFi
        ESP_ERROR_CHECK(esp_wifi_start());

        // Wait until IP is received (only if SSID is not empty - i.e. this is
        // bridge node)
        if (m_ssid.length() > 0) {
            auto future = m_connectingPromise.get_future();

            SPSP_LOGI("Attempting connection with timeout %lld seconds",
                WIFI_INIT_TIMEOUT.count());

            // Block
            if (future.wait_for(WIFI_INIT_TIMEOUT) == std::future_status::timeout) {
                // Connection timeout
                SPSP_LOGE("Connection timeout");
                m_mutex.unlock();
                throw WiFiConnectionError();
            }
        }

        this->m_initialized = true;

        SPSP_LOGI("Initialized");
    }

    void WiFi::initNVS()
    {
        esp_err_t nvsInitCode = nvs_flash_init();

        if (nvsInitCode == ESP_ERR_NVS_NO_FREE_PAGES || nvsInitCode == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            nvsInitCode = nvs_flash_init();
        }

        ESP_ERROR_CHECK(nvsInitCode);

        SPSP_LOGI("NVS initialized");
    }

    void WiFi::initNetIf()
    {
        ESP_ERROR_CHECK(esp_netif_init());

        // Create net interface
        esp_netif_t* netif = esp_netif_create_default_wifi_sta();

        // Set hostname
        std::string hostname = WIFI_HOSTNAME_PREFIX;
        uint8_t mac[8];
        char macStr[16];
        ESP_ERROR_CHECK(esp_efuse_mac_get_default(mac));
        sprintf(macStr, "%02x%02x%02x%02x%02x%02x", MAC2STR(mac));
        hostname += macStr;
        esp_netif_set_hostname(netif, hostname.c_str());

        SPSP_LOGI("Network interface initialized");
    }

    void WiFi::registerEventHandlers()
    {
        // WiFi events
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFi::eventHandlerWiFi,
            this, nullptr));

        // IP events
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            IP_EVENT, ESP_EVENT_ANY_ID, &WiFi::eventHandlerIP,
            this, nullptr));
    }

    void WiFi::initWiFiConfig()
    {
        // SSID is not empty (this is bridge node)
        if (m_ssid.length() > 0) {
            wifi_config_t wifi_config = {};
            strcpy((char*) wifi_config.sta.ssid, m_ssid.c_str());
            strcpy((char*) wifi_config.sta.password, m_password.c_str());

            // Do full scan - connect to AP with strongest signal
            wifi_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;

            // Enable roaming features
            wifi_config.sta.rm_enabled = true;
            wifi_config.sta.btm_enabled = true;
            wifi_config.sta.mbo_enabled = true;
            wifi_config.sta.ft_enabled = true;

            // Enable security features
            wifi_config.sta.owe_enabled = true;
            wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

            // No power saving for bridge (IMPORTANT!)
            esp_wifi_set_ps(WIFI_PS_NONE);
        }
    }

    void WiFi::deinit()
    {
        // Mutex
        const std::lock_guard<std::mutex> lock(m_mutex);

        // Don't do anything if already deinitialized
        if (!this->m_initialized) return;

        ESP_ERROR_CHECK(esp_wifi_stop());
        ESP_ERROR_CHECK(esp_wifi_deinit());
        ESP_ERROR_CHECK(esp_event_loop_delete_default());

        this->m_initialized = false;

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
        // Mutex
        const std::lock_guard lock(m_mutex);

        ESP_ERROR_CHECK(esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE));
        SPSP_LOGI("Set channel %d", ch);
    }
    
    void WiFi::eventHandlerWiFi(void* ctx, esp_event_base_t, int32_t eventId, void* eventData)
    {
        switch (eventId) {
        case WIFI_EVENT_STA_START:
        case WIFI_EVENT_STA_DISCONNECTED:
            esp_wifi_connect();
            break;
        
        default:
            break;
        }
    }

    void WiFi::eventHandlerIP(void* ctx, esp_event_base_t, int32_t eventId, void* eventData)
    {
        // Get "this"
        WiFi* inst = static_cast<WiFi*>(ctx);

        ip_event_got_ip_t* event_got_ip4;
        ip_event_got_ip6_t* event_got_ip6;

        switch (eventId) {
        case IP_EVENT_STA_GOT_IP:
            // Got IPv4 address
            event_got_ip4 = static_cast<ip_event_got_ip_t*>(eventData);
            SPSP_LOGI("Got IP: " IPSTR, IP2STR(&event_got_ip4->ip_info.ip));

            // Resolve promise
            inst->m_connectingPromise.set_value();

            break;
        
        case IP_EVENT_GOT_IP6:
            // Got IPv6 address
            event_got_ip6 = static_cast<ip_event_got_ip6_t*>(eventData);
            SPSP_LOGI("Got IPv6: " IPV6STR, IPV62STR(event_got_ip6->ip6_info.ip));

            // Resolve promise
            inst->m_connectingPromise.set_value();

            break;
        
        default:
            break;
        }
    }
} // namespace SPSP
