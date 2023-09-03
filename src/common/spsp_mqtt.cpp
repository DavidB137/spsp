/**
 * @file spsp_mqtt.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief MQTT far layer for SPSP
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "spsp_logger.hpp"
#include "spsp_mqtt.hpp"

// Log tag
static const char* SPSP_LOG_TAG = "SPSP/Far/MQTT";

namespace SPSP::FarLayers::MQTT
{
    MQTT::MQTT(IAdapter& adapter, const Config& conf)
        : m_conf{conf}, m_adapter{adapter}
    {
        using namespace std::placeholders;

        m_initializing = true;

        // Set adapter callbacks
        m_adapter.setConnectedCb(std::bind(&MQTT::connectedCb, this));
        m_adapter.setSubDataCb(std::bind(&MQTT::subDataCb, this, _1, _2));

        // Wait until connected
        auto future = m_connectingPromise.get_future();
        SPSP_LOGI("Attempting connection with timeout %lld ms",
                  m_conf.connection.timeout.count());

        // Block
        if (future.wait_for(m_conf.connection.timeout) == std::future_status::timeout) {
            // Connection timeout
            SPSP_LOGE("Connection timeout");
            throw ConnectionError("Connection timeout");
        }

        m_initializing = false;

        SPSP_LOGI("Initialized");
    }

    MQTT::~MQTT()
    {
        SPSP_LOGI("Deinitialized");
    }

    bool MQTT::publish(const std::string& src, const std::string& topic,
                        const std::string& payload)
    {
        SPSP_LOGD("Publish: payload '%s' to topic '%s' from %s",
                  payload.c_str(), topic.c_str(), src.c_str());

        std::string topicExtended = m_conf.pubTopicPrefix + "/" + src + "/"
                                  + topic;

        return m_adapter.publish(topicExtended, payload);
    }

    bool MQTT::subscribe(const std::string& topic)
    {
        SPSP_LOGD("Subscribe to topic '%s'", topic.c_str());

        // Subscribe (blocks)
        return m_adapter.subscribe(topic);
    }

    bool MQTT::unsubscribe(const std::string& topic)
    {
        SPSP_LOGD("Unsubscribe from topic '%s'", topic.c_str());

        // Unsubscribe (blocks)
        return m_adapter.unsubscribe(topic);
    }

    void MQTT::connectedCb()
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

    void MQTT::subDataCb(const std::string& topic, const std::string& payload)
    {
        if (this->nodeConnected()) {
            this->getNode()->receiveFar(topic, payload);
        }
    }
}
