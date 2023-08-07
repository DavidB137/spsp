/**
 * @file spsp_wifi.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief WiFi manager for ESP platform
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <chrono>
#include <exception>
#include <future>
#include <mutex>
#include <string>

#include "esp_event.h"
#include "esp_netif.h"

namespace SPSP
{
    const char* const WIFI_HOSTNAME_PREFIX = "spsp-";         //!< Hostname prefix
    const auto WIFI_INIT_TIMEOUT = std::chrono::seconds(20);  //!< Timeout for connecting to AP
    const int WIFI_TX_POWER_DEFAULT = INT_MIN;                //!< Default TX power

    /**
     * @brief WiFi connection error
     * 
     * Thrown when `WIFI_INIT_TIMEOUT` expires before successful connection.
     */
    class WiFiConnectionError : public std::exception {};

    /**
     * @brief ESP-NOW configuration
     * 
     */
    struct WiFiConfig
    {
        std::string ssid = "";                   //!< SSID
        std::string password = "";               //!< Password
        bool lockBssid = false;                  //!< Whether to use only AP with MAC address `bssid`
        bool enableIPv6 = false;                 //!< Whether to enable IPv6 addressing
        uint8_t bssid[6];                        //!< MAC address of AP
        int maxTxPower = WIFI_TX_POWER_DEFAULT;  //!< Maximum transmit power (in dBm)
    };

    /**
     * @brief WiFi manager for ESP platform (singleton)
     * 
     * Must be initialized manually before instantiating ESP-NOW and MQTT.
     * (And deinitialized also manually.)
     * 
     * In WiFi station mode, all roaming features and WPA3 are enabled.
     */
    class WiFi
    {
        WiFiConfig m_config = {};                //!< Config
        esp_netif_t* m_netIf = nullptr;          //!< Network interface pointer
        bool m_initialized = false;              //!< Whether WiFi is initialized
        std::promise<void> m_connectingPromise;  //!< Promise to block until successful connection is made
        std::mutex m_mutex;                      //!< Mutex to prevent race conditions (primarily for initialization)

        WiFi() {}

    public:
        /**
         * @brief Returns instance of this singleton class
         * 
         * @return This instance
         */
        static WiFi& getInstance()
        {
            static WiFi inst;
            return inst;
        }

        /**
         * @brief Initializes WiFi
         * 
         * If SSID is zero-length (or not given), no net interface is created
         * and WiFi is initalized in ESP-NOW-only mode.
         * 
         * May be called multiple times and is multi-thread safe.
         * Blocks until connection is established. May throw
         * `WiFiConnectionError`.
         * 
         * @param config Configuration
         */
        void init(const WiFiConfig config = {});

        /**
         * @brief Deinitializes WiFi
         * 
         * May be called multiple times.
         */
        void deinit();

        /**
         * @brief Gets current WiFi channel
         * 
         * @return Channel number
         */
        uint8_t getChannel();

        /**
         * @brief Sets current channel
         * 
         * @param ch Channel
         */
        void setChannel(uint8_t ch);

        /**
         * @brief Sets country restrictions
         * 
         * Calling this method is not needed on bridge node.
         * 
         * @param cc Country code
         * @param lowCh Lowest allowed channel
         * @param highCh Highest allowed channel
         */
        void setCountryRestrictions(const char cc[3], uint8_t lowCh,
                                    uint8_t highCh);

        /**
         * @brief Creates IPv6 link-local address
         * 
         * If IPv6 is disabled in config, does nothing.
         */
        void createIPv6LinkLocal();

    private:
        /**
         * @brief Initializes NVS
         * 
         * Called from `init()`.
         */
        void initNVS();

        /**
         * @brief Initializes network interface.
         * 
         * Called from `init()`.
         */
        void initNetIf();

        /**
         * @brief Registers WiFi event handlers.
         * 
         * Called from `init()`.
         */
        void registerEventHandlers();

        /**
         * @brief Initializes WiFi configuration to match current parameters.
         * 
         * Called from `init()`.
         */
        void initWiFiConfig();

        /**
         * @brief Event handler for WiFi
         * 
         * @param ctx Context (pointer to this instance)
         * @param eventBase Event base (`WIFI_EVENT`)
         * @param eventId Event ID
         * @param eventData Data of that event
         */
        static void eventHandlerWiFi(void* ctx, esp_event_base_t eventBase,
                                     int32_t eventId, void* eventData);

        /**
         * @brief Event handler for IP
         * 
         * @param ctx Context (pointer to this instance)
         * @param eventBase Event base (`IP_EVENT`)
         * @param eventId Event ID
         * @param eventData Data of that event
         */
        static void eventHandlerIP(void* ctx, esp_event_base_t eventBase,
                                   int32_t eventId, void* eventData);
    };
} // namespace SPSP
