/**
 * @file spsp_mqtt_adapter.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief MQTT far layer for SPSP
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <chrono>
#include <thread>

#include "esp_mac.h"

#include "spsp_exception_check.hpp"
#include "spsp_logger.hpp"
#include "spsp_mac.hpp"
#include "spsp_mqtt_adapter.hpp"

// Log tag
static const char* SPSP_LOG_TAG = "SPSP/Far/MQTT/Adapter";

namespace SPSP::FarLayers::MQTT
{
    void _eventHandler(void *ctx, esp_event_base_t, int32_t eventId, void *eventData)
    {
        auto inst = static_cast<Adapter*>(ctx);
        auto event = static_cast<esp_mqtt_event_handle_t>(eventData);

        std::string topic;
        std::string data;

        switch (eventId) {
        case MQTT_EVENT_CONNECTED:
            SPSP_LOGD("Connected");
            if (inst->getConnectedCb() != nullptr) {
                inst->getConnectedCb()();
            }
            break;

        case MQTT_EVENT_DISCONNECTED:
            SPSP_LOGW("Disconnected, MQTT will reconnect automatically...");
            break;

        case MQTT_EVENT_DATA:
            // TODO: implement support for multiple chunks of data
            if (event->current_data_offset > 0) return;

            topic = std::string(event->topic, event->topic_len);
            data = std::string(event->data, event->data_len);

            SPSP_LOGD("Received data '%s' on topic '%s'",
                      data.c_str(), topic.c_str());

            // Call subscription data callback if defined
            if (inst->getSubDataCb() != nullptr) {
                std::thread t(inst->getSubDataCb(), topic, data);
                t.detach();
            }
            break;

        default:
            break;
        }
    }

    Adapter::Adapter(const Config& conf)
        : m_conf{conf}
    {
        // Client ID
        std::string clientId;

        if (m_conf.auth.clientId.empty()) {
            clientId = MQTT_CLIENT_ID_PREFIX;

            uint8_t mac[8];
            char macStr[16];
            getLocalMAC(mac);
            sprintf(macStr, "%02x%02x%02x%02x%02x%02x", MAC2STR(mac));

            clientId += macStr;
        } else {
            clientId = m_conf.auth.clientId;
        }

        // Create ESP config
        esp_mqtt_client_config_t espConfig = {};

        espConfig.broker.address.uri = m_conf.connection.uri.c_str();
        espConfig.broker.verification.certificate = this->stringToCOrNull(m_conf.connection.verifyCrt);
        espConfig.credentials.username = m_conf.auth.username.c_str();
        espConfig.credentials.client_id = clientId.c_str();
        espConfig.credentials.authentication.password = this->stringToCOrNull(m_conf.auth.password);
        espConfig.credentials.authentication.certificate = this->stringToCOrNull(m_conf.auth.crt);
        espConfig.credentials.authentication.key = this->stringToCOrNull(m_conf.auth.crtKey);
        espConfig.session.last_will.topic = this->stringToCOrNull(m_conf.lastWill.topic);
        espConfig.session.last_will.msg = this->stringToCOrNull(m_conf.lastWill.msg);
        espConfig.session.last_will.qos = m_conf.lastWill.qos;
        espConfig.session.last_will.retain = m_conf.lastWill.retain;
        espConfig.session.keepalive = m_conf.connection.keepalive;
        espConfig.session.disable_keepalive = m_conf.connection.keepalive <= 0;

        // Initialize client
        m_mqtt = esp_mqtt_client_init(&espConfig);

        // Register event handler
        SPSP_ERROR_CHECK(
            esp_mqtt_client_register_event(
                m_mqtt,
                static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID),
                _eventHandler,
                this
            ),
            AdapterError("Event handler register failed")
        );

        // Start the client
        SPSP_ERROR_CHECK(
            esp_mqtt_client_start(m_mqtt),
            AdapterError("Client start failed")
        );
    }

    Adapter::~Adapter()
    {
        esp_mqtt_client_stop(m_mqtt);
        esp_mqtt_client_destroy(m_mqtt);
    }

    bool Adapter::publish(const std::string& topic, const std::string& payload)
    {
        return esp_mqtt_client_enqueue(m_mqtt, topic.c_str(), payload.c_str(),
                                       payload.length(), m_conf.connection.qos,
                                       m_conf.connection.retain, true) > 0;
    }

    bool Adapter::subscribe(const std::string& topic)
    {
        return esp_mqtt_client_subscribe(m_mqtt, topic.c_str(),
                                         m_conf.connection.qos) > 0;
    }

    bool Adapter::unsubscribe(const std::string& topic)
    {
        return esp_mqtt_client_unsubscribe(m_mqtt, topic.c_str()) > 0;
    }

    void Adapter::setSubDataCb(AdapterSubDataCb cb)
    {
        m_subDataCb = cb;
    }

    AdapterSubDataCb Adapter::getSubDataCb() const
    {
        return m_subDataCb;
    }

    void Adapter::setConnectedCb(AdapterConnectedCb cb)
    {
        m_connectedCb = cb;
    }

    AdapterConnectedCb Adapter::getConnectedCb() const
    {
        return m_connectedCb;
    }
}
