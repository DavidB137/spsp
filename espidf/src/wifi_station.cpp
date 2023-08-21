/**
 * @file wifi_station.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief WiFi station for ESP platform
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <cstring>

#include "esp_mac.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "spsp_logger.hpp"
#include "spsp_wifi_station.hpp"

// Log tag
static const char* SPSP_LOG_TAG = "SPSP/WiFi";

namespace SPSP::WiFi
{
    Station::Station(const StationConfig& config)
    {
        // Mutex
        const std::lock_guard<std::mutex> lock(m_mutex);

        // Don't do anything if already initialized
        if (m_initialized) return;

        m_config = config;

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

        // Set TX power
        if (config.maxTxPower != TX_POWER_DEFAULT) {
            ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(4 * config.maxTxPower));
        }

        // Wait until IP is received (only if SSID is not empty - i.e. this is
        // bridge node)
        if (config.ssid.length() > 0) {
            auto future = m_connectingPromise.get_future();

            SPSP_LOGI("Attempting connection with timeout %lld seconds",
                      m_config.initTimeout.count());

            // Block
            if (future.wait_for(m_config.initTimeout) == std::future_status::timeout) {
                // Connection timeout
                SPSP_LOGE("Connection timeout");
                m_mutex.unlock();
                throw ConnectionError("Connection timeout");
            }
        }

        m_initialized = true;

        SPSP_LOGI("Initialized");
    }

    Station::~Station()
    {
        // Mutex
        const std::lock_guard<std::mutex> lock(m_mutex);

        // Don't do anything if already deinitialized
        if (!m_initialized) return;

        ESP_ERROR_CHECK(esp_wifi_stop());
        ESP_ERROR_CHECK(esp_wifi_deinit());
        ESP_ERROR_CHECK(esp_event_loop_delete_default());

        m_initialized = false;

        SPSP_LOGI("Deinitialized");
    }

    uint8_t Station::getChannel() const
    {
        // Mutex
        const std::lock_guard lock(m_mutex);

        uint8_t ch;
        wifi_second_chan_t sec;
        ESP_ERROR_CHECK(esp_wifi_get_channel(&ch, &sec));
        return ch;
    }

    void Station::setChannel(uint8_t ch)
    {
        // Mutex
        const std::lock_guard lock(m_mutex);

        ESP_ERROR_CHECK(esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE));
        SPSP_LOGI("Set channel %d", ch);
    }

    void Station::setChannelRestrictions(const ChannelRestrictions& rest)
    {
        // Mutex
        const std::lock_guard lock(m_mutex);

        wifi_country_t c = {
            .cc = "XX",
            .schan = rest.low,
            .nchan = rest.high - rest.low + 1,
        };

        ESP_ERROR_CHECK(esp_wifi_set_country(&c));

        SPSP_LOGI("Set channel restrictions: %d - %d", rest.low, rest.high);
    }

    const ChannelRestrictions Station::getChannelRestrictions() const
    {
        wifi_country_t c;
        ESP_ERROR_CHECK(esp_wifi_get_country(&c));

        return {
            .low = c.schan,
            .high = c.schan + c.nchan - 1
        };
    }

    void Station::createIPv6LinkLocal()
    {
        if (!m_config.enableIPv6) {
            return;
        }

        ESP_ERROR_CHECK(esp_netif_create_ip6_linklocal(m_netIf));
    }

    void Station::initNVS()
    {
        esp_err_t nvsInitCode = nvs_flash_init();

        if (nvsInitCode == ESP_ERR_NVS_NO_FREE_PAGES ||
            nvsInitCode == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            nvsInitCode = nvs_flash_init();
        }

        ESP_ERROR_CHECK(nvsInitCode);

        SPSP_LOGI("NVS initialized");
    }

    void Station::initNetIf()
    {
        // Don't do anything if SSID is empty
        if (m_config.ssid.length() == 0) return;

        ESP_ERROR_CHECK(esp_netif_init());

        // Create net interface
        m_netIf = esp_netif_create_default_wifi_sta();

        // Set hostname
        std::string hostname = m_config.hostnamePrefix;
        uint8_t mac[8];
        char macStr[16];
        ESP_ERROR_CHECK(esp_efuse_mac_get_default(mac));
        sprintf(macStr, "%02x%02x%02x%02x%02x%02x", MAC2STR(mac));
        hostname += macStr;
        esp_netif_set_hostname(m_netIf, hostname.c_str());

        SPSP_LOGI("Network interface initialized");
    }

    void Station::registerEventHandlers()
    {
        // WiFi events
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            WIFI_EVENT, ESP_EVENT_ANY_ID, &Station::eventHandlerWiFi,
            this, nullptr));

        // IP events
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            IP_EVENT, ESP_EVENT_ANY_ID, &Station::eventHandlerIP,
            this, nullptr));
    }

    void Station::initWiFiConfig()
    {
        // SSID is not empty (this is bridge node)
        if (m_config.ssid.length() > 0) {
            wifi_config_t espWiFiConfig = {};
            strcpy((char*) espWiFiConfig.sta.ssid, m_config.ssid.c_str());
            strcpy((char*) espWiFiConfig.sta.password, m_config.password.c_str());

            // BSSID
            espWiFiConfig.sta.bssid_set = m_config.lockBssid;
            memcpy(espWiFiConfig.sta.bssid, m_config.bssid, sizeof(m_config.bssid));

            // Do full scan - connect to AP with strongest signal
            espWiFiConfig.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;

            // Enable roaming features
            espWiFiConfig.sta.rm_enabled = true;
            espWiFiConfig.sta.btm_enabled = true;
            espWiFiConfig.sta.mbo_enabled = true;
            espWiFiConfig.sta.ft_enabled = true;

            // Enable security features
            espWiFiConfig.sta.owe_enabled = true;
            espWiFiConfig.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &espWiFiConfig));

            // No power saving for bridge (IMPORTANT!)
            esp_wifi_set_ps(WIFI_PS_NONE);
        }
    }

    void Station::eventHandlerWiFi(void* ctx, esp_event_base_t eventBase,
                                    int32_t eventId, void* eventData)
    {
        // Get "this"
        Station* inst = static_cast<Station*>(ctx);

        switch (eventId) {
        case WIFI_EVENT_STA_START:
        case WIFI_EVENT_STA_DISCONNECTED:
            esp_wifi_connect();
            break;

        case WIFI_EVENT_STA_CONNECTED:
            inst->createIPv6LinkLocal();
            break;

        default:
            break;
        }
    }

    void Station::eventHandlerIP(void* ctx, esp_event_base_t eventBase,
                                int32_t eventId, void* eventData)
    {
        // Get "this"
        Station* inst = static_cast<Station*>(ctx);

        ip_event_got_ip_t* eventGotIPv4;
        ip_event_got_ip6_t* eventGotIPv6;
        esp_ip6_addr_type_t ipv6AddrType;

        switch (eventId) {
        case IP_EVENT_STA_GOT_IP:
            // Got IPv4 address
            eventGotIPv4 = static_cast<ip_event_got_ip_t*>(eventData);
            SPSP_LOGI("Got IP: " IPSTR, IP2STR(&eventGotIPv4->ip_info.ip));

            // Resolve promise
            if (!inst->m_initialized) {
                inst->m_connectingPromise.set_value();
            }

            break;

        case IP_EVENT_GOT_IP6:
            // Got IPv6 address
            eventGotIPv6 = static_cast<ip_event_got_ip6_t*>(eventData);
            SPSP_LOGI("Got IPv6: " IPV6STR, IPV62STR(eventGotIPv6->ip6_info.ip));

            ipv6AddrType = esp_netif_ip6_get_addr_type(&(eventGotIPv6->ip6_info.ip));

            // Resolve promise if it's global address
            if (!inst->m_initialized && ipv6AddrType == ESP_IP6_ADDR_IS_GLOBAL) {
                inst->m_connectingPromise.set_value();
            }

            break;

        default:
            break;
        }
    }
} // namespace SPSP::WiFi
