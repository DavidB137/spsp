/**
 * @file mqtt.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief MQTT far layer for SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <chrono>
#include <thread>

#include "esp_mac.h"
#include "mqtt_client.h"

#include "spsp_logger.hpp"
#include "spsp_mqtt.hpp"
#include "spsp_wifi.hpp"

// Log tag
static const char* SPSP_LOG_TAG = "SPSP/Far/MQTT";

namespace SPSP::FarLayers::MQTT
{
    void _eventHandler(void *ctx, esp_event_base_t, int32_t eventId, void *eventData)
    {
        using namespace std::chrono_literals;

        auto mqtt = static_cast<Layer*>(ctx);
        auto event = static_cast<esp_mqtt_event_handle_t>(eventData);

        std::string topic;
        std::string data;

        switch (eventId) {
        case MQTT_EVENT_CONNECTED:
            SPSP_LOGD("Connected");
            mqtt->connected();
            break;

        case MQTT_EVENT_DISCONNECTED:
            SPSP_LOGD("Disconnected, MQTT will reconnect automatically...");
            break;

        case MQTT_EVENT_DATA:
            // TODO: implement support for multiple chunks of data
            if (event->current_data_offset > 0) return;

            topic = std::string(event->topic, event->topic_len);
            data = std::string(event->data, event->data_len);

            SPSP_LOGD("Received data '%s' on topic '%s'",
                      data.c_str(), topic.c_str());

            if (mqtt->nodeConnected()) {
                // Create new thread
                std::thread t([mqtt, topic, data] {
                    mqtt->getNode()->receiveFar(topic, data);
                });

                // Run independently
                t.detach();
            }
            break;

        default:
            break;
        }
    }

    Layer::Layer(const ClientConfig config)
        : m_pubTopicPrefix{config.pubTopicPrefix}, m_qos{config.connection.qos},
          m_retain{config.connection.retain}
    {
        m_initializing = true;

        // Client ID
        std::string clientId;

        if (config.auth.clientId.empty()) {
            clientId = MQTT_CLIENT_ID_PREFIX;

            uint8_t mac[8];
            char macStr[16];
            ESP_ERROR_CHECK(esp_efuse_mac_get_default(mac));
            sprintf(macStr, "%02x%02x%02x%02x%02x%02x", MAC2STR(mac));

            clientId += macStr;
        } else {
            clientId = config.auth.clientId;
        }

        // Create ESP config
        esp_mqtt_client_config_t espConfig = {};

        espConfig.broker.address.uri = config.connection.uri.c_str();
        espConfig.broker.verification.certificate = this->stringToCOrNull(config.connection.verifyCrt);
        espConfig.credentials.username = config.auth.username.c_str();
        espConfig.credentials.client_id = clientId.c_str();
        espConfig.credentials.authentication.password = this->stringToCOrNull(config.auth.password);
        espConfig.credentials.authentication.certificate = this->stringToCOrNull(config.auth.crt);
        espConfig.credentials.authentication.key = this->stringToCOrNull(config.auth.crtKey);
        espConfig.session.last_will.topic = this->stringToCOrNull(config.lastWill.topic);
        espConfig.session.last_will.msg = this->stringToCOrNull(config.lastWill.msg);
        espConfig.session.last_will.qos = config.lastWill.qos;
        espConfig.session.last_will.retain = config.lastWill.retain;
        espConfig.session.keepalive = config.connection.keepalive;
        espConfig.session.disable_keepalive = config.connection.keepalive <= 0;

        // Initialize client
        m_mqtt = esp_mqtt_client_init(&espConfig);

        // Register event handler
        ESP_ERROR_CHECK(
            esp_mqtt_client_register_event(
                static_cast<esp_mqtt_client_handle_t>(m_mqtt),
                static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID),
                _eventHandler,
                this
            )
        );

        // Start the client
        ESP_ERROR_CHECK(
            esp_mqtt_client_start(static_cast<esp_mqtt_client_handle_t>(m_mqtt))
        );

        // Wait until connected
        auto future = m_connectingPromise.get_future();
        SPSP_LOGI("Attempting connection with timeout %lld seconds",
                MQTT_CONNECT_TIMEOUT.count());

        // Block
        if (future.wait_for(MQTT_CONNECT_TIMEOUT) == std::future_status::timeout) {
            // Connection timeout
            SPSP_LOGE("Connection timeout");
            throw MQTTConnectionError();
        }

        m_initializing = false;

        SPSP_LOGI("Initialized");
    }

    Layer::~Layer()
    {
        auto mqtt = static_cast<esp_mqtt_client_handle_t>(m_mqtt);

        ESP_ERROR_CHECK(esp_mqtt_client_stop(mqtt));
        ESP_ERROR_CHECK(esp_mqtt_client_destroy(mqtt));

        SPSP_LOGI("Deinitialized");
    }

    void Layer::connected()
    {
        if (m_initializing) {
            m_connectingPromise.set_value();
        } else {
            // Successful reconnection
            // Resubscribe to all topics
            if (this->nodeConnected()) {
                this->getNode()->resubscribeAll();
            }
        }
    }

    bool Layer::publish(const std::string src, const std::string topic,
                        const std::string payload)
    {
        SPSP_LOGD("Publish: payload '%s' to topic '%s' from %s",
                  payload.c_str(), topic.c_str(), src.c_str());

        std::string topicExtended = m_pubTopicPrefix + "/" + src + "/" + topic;

        // No-blocking publish
        auto mqtt = static_cast<esp_mqtt_client_handle_t>(m_mqtt);
        esp_mqtt_client_enqueue(mqtt, topicExtended.c_str(), payload.c_str(),
                                payload.length(), m_qos, m_retain, true);

        return true;
    }

    bool Layer::subscribe(const std::string topic)
    {
        SPSP_LOGD("Subscribe to topic '%s'", topic.c_str());

        // Subscribe (blocks)
        auto mqtt = static_cast<esp_mqtt_client_handle_t>(m_mqtt);
        return esp_mqtt_client_subscribe(mqtt, topic.c_str(), m_qos) > 0;
    }

    bool Layer::unsubscribe(const std::string topic)
    {
        SPSP_LOGD("Unsubscribe from topic '%s'", topic.c_str());
        
        // Unsubscribe (blocks)
        auto mqtt = static_cast<esp_mqtt_client_handle_t>(m_mqtt);
        return esp_mqtt_client_unsubscribe(mqtt, topic.c_str()) > 0;
    }
}
