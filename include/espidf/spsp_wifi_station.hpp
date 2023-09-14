/**
 * @file spsp_wifi_station.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief WiFi station for ESP platform
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

#include "spsp_wifi_espnow_if.hpp"
#include "spsp_wifi_types.hpp"

namespace SPSP::WiFi
{
    /**
     * @brief WiFi station configuration
     *
     */
    struct StationConfig
    {
        // Connection
        std::string ssid = "";      //!< SSID
        std::string password = "";  //!< Password
        bool lockBssid = false;     //!< Whether to use only AP with MAC address `bssid`
        uint8_t bssid[6];           //!< MAC address of AP

        // Network
        std::string hostnamePrefix = "spsp-";  //!< Hostname prefix (followed by MAC address)
        bool enableIPv6 = false;               //!< Whether to enable IPv6 addressing (waits for either IPv4 or *global* IPv6 address)

        // Signal
        int maxTxPower = TX_POWER_DEFAULT;     //!< Maximum transmit power (in dBm)

        // Timing
        std::chrono::milliseconds initTimeout = std::chrono::seconds(20);  //!< Timeout for connecting to AP
    };

    /**
     * @brief WiFi station for ESP platform
     *
     * There may be only one instance at a time (despite of not being
     * implemented as singleton).
     *
     * If config contains empty SSID, only functions as ESP-NOW transceiver.
     *
     * All roaming features and WPA3 are enabled by default.
     *
     * Implements ESP-NOW interface requirements.
     */
    class Station : public IESPNOW
    {
        std::mutex m_mutex;                      //!< Mutex to prevent race conditions (primarily for initialization)
        StationConfig m_config = {};             //!< Config
        esp_netif_t* m_netIf = nullptr;          //!< Network interface pointer
        bool m_initialized = false;              //!< Whether WiFi is initialized
        std::promise<void> m_connectingPromise;  //!< Promise to block until successful connection is made

    public:
        /**
         * @brief Constructs new WiFi station object and initializes connection
         *
         * @param config Configuration
         * @throw ConnectionError when connection cannot be established within
         *        configured timeout
         */
        Station(const StationConfig& config);

        /**
         * @brief Disconnects from AP and destroys WiFi station
         *
         */
        virtual ~Station();

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
         * @brief Sets channel restrictions
         *
         * Set lowest and highest usable WiFi channel
         * (legal country restrictions).
         *
         * @param rest Restrictions
         */
        void setChannelRestrictions(const ChannelRestrictions& rest);

        /**
         * @brief Get the Channel Restrictions object
         * 
         * @return const ChannelRestrictions 
         */
        const ChannelRestrictions getChannelRestrictions();

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
} // namespace SPSP::WiFi
