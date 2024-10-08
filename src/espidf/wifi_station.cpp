/**
 * @file wifi_station.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief WiFi station for ESP platform
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <cstring>

#include "esp_eap_client.h"
#include "esp_mac.h"
#include "esp_netif_sntp.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "spsp/exception_check.hpp"
#include "spsp/logger.hpp"
#include "spsp/wifi_station.hpp"

// Log tag
static const char* SPSP_LOG_TAG = "SPSP/WiFi/Station";

namespace SPSP::WiFi
{
    Station::Station(const StationConfig& config)
        : m_config{config}
    {
        // Mutex
        const std::scoped_lock<std::mutex> lock(m_mutex);

        // Create event loop
        SPSP_ERROR_CHECK(esp_event_loop_create_default(),
                         ConnectionError("Create event loop failed"));

        // Initialize NVS and network
        this->initNVS();
        this->initNetIf();

        // Register handlers
        this->registerEventHandlers();

        // Init configuration
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        SPSP_ERROR_CHECK(esp_wifi_init(&cfg),
                         ConnectionError("WiFi init failed"));
        SPSP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM),
                         ConnectionError("Set WiFi storage failed"));
        SPSP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA),
                         ConnectionError("Set WiFi mode failed"));

        // Use current config settings
        this->initWiFiConfig();

        // Start WiFi
        SPSP_ERROR_CHECK(esp_wifi_start(),
                         ConnectionError("WiFi start failed"));

        // Set TX power
        if (config.maxTxPower != TX_POWER_DEFAULT) {
            SPSP_ERROR_CHECK(esp_wifi_set_max_tx_power(4 * config.maxTxPower),
                             ConnectionError("Set max TX power failed"));
        }

        // Wait until IP is received (only if SSID is not empty - i.e. this is
        // bridge node)
        if (!config.ssid.empty()) {
            auto future = m_connectingPromise.get_future();

            SPSP_LOGI("Attempting connection with timeout %lld ms",
                      m_config.initTimeout.count());

            // Block
            if (future.wait_for(m_config.initTimeout) == std::future_status::timeout) {
                // Connection timeout
                SPSP_LOGE("Connection timeout");
                m_mutex.unlock();
                throw ConnectionError("Connection timeout");
            }

            // Synchronize time
            SPSP_LOGI("Attempting time sync with timeout %lld ms",
                      m_config.sntpTimeout.count());

            esp_sntp_config_t sntpConfig = {};  // ESP_NETIF_SNTP_DEFAULT_CONFIG doesn't work
            sntpConfig.wait_for_sync = true;
            sntpConfig.start = true;
            sntpConfig.num_of_servers = 1;
            sntpConfig.servers[0] = m_config.sntpServer.c_str();

            SPSP_ERROR_CHECK(esp_netif_sntp_init(&sntpConfig),
                             ConnectionError("SNTP init failed"));

            // Block
            SPSP_ERROR_CHECK(
                esp_netif_sntp_sync_wait(pdMS_TO_TICKS(m_config.sntpTimeout.count())),
                ConnectionError("SNTP synchronization timeout")
            );
        }

        m_initialized = true;

        SPSP_LOGI("Initialized");
    }

    Station::~Station()
    {
        // Mutex
        const std::scoped_lock<std::mutex> lock(m_mutex);

        esp_netif_sntp_deinit();
        esp_wifi_stop();
        esp_wifi_deinit();
        esp_event_loop_delete_default();

        SPSP_LOGI("Deinitialized");
    }

    uint8_t Station::getChannel()
    {
        // Mutex
        const std::scoped_lock lock(m_mutex);

        uint8_t ch;
        wifi_second_chan_t sec;
        SPSP_ERROR_CHECK(esp_wifi_get_channel(&ch, &sec),
                         ConnectionError("Get WiFi channel failed"));
        return ch;
    }

    void Station::setChannel(uint8_t ch)
    {
        // Mutex
        const std::scoped_lock lock(m_mutex);

        SPSP_ERROR_CHECK(esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE),
                         ConnectionError("Set WiFi channel failed"));
        SPSP_LOGD("Set channel %d", ch);
    }

    void Station::setChannelRestrictions(const ChannelRestrictions& rest)
    {
        // Mutex
        const std::scoped_lock lock(m_mutex);

        wifi_country_t c = {};
        c.cc[0] = '0';
        c.cc[1] = '1';
        c.schan = rest.low;
        c.nchan = rest.high - rest.low + 1;

        SPSP_ERROR_CHECK(esp_wifi_set_country(&c),
                         ConnectionError("Set WiFi country restrictions failed"));

        SPSP_LOGI("Set channel restrictions: %d - %d", rest.low, rest.high);
    }

    const ChannelRestrictions Station::getChannelRestrictions()
    {
        // Mutex
        const std::scoped_lock lock(m_mutex);

        wifi_country_t c;
        SPSP_ERROR_CHECK(esp_wifi_get_country(&c),
                         ConnectionError("Get country restrictions failed"));

        ChannelRestrictions rest = {};
        rest.low = c.schan;
        rest.high = c.schan + c.nchan - 1;
        return rest;
    }

    void Station::createIPv6LinkLocal()
    {
        if (!m_config.enableIPv6) {
            return;
        }

        SPSP_ERROR_CHECK(esp_netif_create_ip6_linklocal(m_netIf),
                         ConnectionError("Create IPv6 link-local failed"));
    }

    void Station::initNVS()
    {
        esp_err_t nvsInitCode = nvs_flash_init();

        if (nvsInitCode == ESP_ERR_NVS_NO_FREE_PAGES ||
            nvsInitCode == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            SPSP_ERROR_CHECK(nvs_flash_erase(),
                             ConnectionError("NVS flash erase failed"));
            nvsInitCode = nvs_flash_init();
        }

        SPSP_ERROR_CHECK(nvsInitCode, ConnectionError("NVS init failed"));

        SPSP_LOGD("NVS initialized");
    }

    void Station::initNetIf()
    {
        // Don't do anything if SSID is empty
        if (m_config.ssid.empty()) return;

        SPSP_ERROR_CHECK(esp_netif_init(),
                         ConnectionError("Network interface init failed"));

        // Create net interface
        m_netIf = esp_netif_create_default_wifi_sta();

        // Set hostname
        std::string hostname = m_config.hostnamePrefix;
        uint8_t mac[8];
        char macStr[16];
        esp_efuse_mac_get_default(mac);
        sprintf(macStr, "%02x%02x%02x%02x%02x%02x", MAC2STR(mac));
        hostname += macStr;
        esp_netif_set_hostname(m_netIf, hostname.c_str());

        SPSP_LOGD("Network interface initialized");
    }

    void Station::registerEventHandlers()
    {
        // WiFi events
        SPSP_ERROR_CHECK(esp_event_handler_instance_register(
            WIFI_EVENT, ESP_EVENT_ANY_ID, &Station::eventHandlerWiFi,
            this, nullptr),
            ConnectionError("Register WiFi event handler failed"));

        // IP events
        SPSP_ERROR_CHECK(esp_event_handler_instance_register(
            IP_EVENT, ESP_EVENT_ANY_ID, &Station::eventHandlerIP,
            this, nullptr),
            ConnectionError("Register IP event handler failed"));
    }

    void Station::initWiFiConfig()
    {
        // SSID is not empty (this is bridge node)
        if (!m_config.ssid.empty()) {
            wifi_config_t espWiFiConfig = {};
            strcpy((char*) espWiFiConfig.sta.ssid, m_config.ssid.c_str());
            strcpy((char*) espWiFiConfig.sta.password, m_config.password.c_str());

            // BSSID
            espWiFiConfig.sta.bssid_set = m_config.lockBssid;
            memcpy(espWiFiConfig.sta.bssid, m_config.bssid,
                   sizeof(m_config.bssid));

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

            SPSP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &espWiFiConfig),
                             ConnectionError("Set WiFi config failed"));
            if (!m_config.eapIdentity.empty()) {
                SPSP_ERROR_CHECK(
                    esp_eap_client_set_identity(
                        (const uint8_t*)m_config.eapIdentity.c_str(),
                        m_config.eapIdentity.length()
                    ),
                    ConnectionError("Set EAP identity failed")
                );
            }
            if (!m_config.eapCACrt.empty()) {
                SPSP_ERROR_CHECK(
                    // Has to be NULL-terminated
                    esp_eap_client_set_ca_cert(
                        (const uint8_t*)m_config.eapCACrt.c_str(),
                        m_config.eapCACrt.length() + 1
                    ),
                    ConnectionError("Set EAP CA certificate failed")
                );
            }
            if (!m_config.eapCrt.empty() && !m_config.eapCrtKey.empty()) {
                SPSP_ERROR_CHECK(
                    // Has to be NULL-terminated
                    esp_eap_client_set_certificate_and_key(
                        (const uint8_t*)m_config.eapCrt.c_str(),
                        m_config.eapCrt.length() + 1,
                        (const uint8_t*)m_config.eapCrtKey.c_str(),
                        m_config.eapCrtKey.length() + 1, nullptr, 0
                    ),
                    ConnectionError("Set EAP client certificate failed")
                );
            }
            if (!m_config.eapUsername.empty()) {
                SPSP_ERROR_CHECK(
                    esp_eap_client_set_username(
                        (const uint8_t*)m_config.eapUsername.c_str(),
                        m_config.eapUsername.length()
                    ),
                    ConnectionError("Set EAP username failed")
                );
            }
            if (!m_config.eapPassword.empty()) {
                SPSP_ERROR_CHECK(
                    esp_eap_client_set_password(
                        (const uint8_t*)m_config.eapPassword.c_str(),
                        m_config.eapPassword.length()
                    ),
                    ConnectionError("Set EAP password failed")
                );
            }
            if (m_config.enableWPA3EAP192bit) {
                SPSP_ERROR_CHECK(
                    esp_eap_client_set_suiteb_192bit_certification(true),
                    ConnectionError("Enable EAP WPA3-192 failed")
                );
            }
            if (m_config.enableWPAEnterprise) {
                SPSP_ERROR_CHECK(
                    esp_wifi_sta_enterprise_enable(),
                    ConnectionError("Enable WPA Enterprise failed")
                );
            }

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
            if (!inst->m_config.ssid.empty()) {
                esp_wifi_connect();
            }
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
