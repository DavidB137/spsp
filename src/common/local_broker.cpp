/**
 * @file local_broker.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Local broker far layer for SPSP
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <thread>

#include "spsp/local_broker.hpp"
#include "spsp/logger.hpp"

// Log tag
static const char* SPSP_LOG_TAG = "SPSP/Far/LocalBroker";

namespace SPSP::FarLayers::LocalBroker
{
    LocalBroker::LocalBroker(const std::string topicPrefix)
        : m_topicPrefix{topicPrefix}
    {
        SPSP_LOGI("Initialized");
    }

    LocalBroker::~LocalBroker()
    {
        SPSP_LOGI("Deinitialized");
    }

    bool LocalBroker::publish(const std::string& src, const std::string& topic,
                        const std::string& payload)
    {
        SPSP_LOGD("Publish: payload '%s' to topic '%s' from %s",
                  payload.c_str(), topic.c_str(), src.c_str());

        std::string topicPrefix = "";
        if (m_topicPrefix.length() > 0) {
            topicPrefix = m_topicPrefix + "/";
        }

        std::string topicExtended = topicPrefix + src + "/" + topic;

        // Check if node is subscribed to this topic
        bool subscribed;
        {
            const std::scoped_lock lock(m_mutex);
            subscribed = !m_subs.find(topicExtended).empty();
        }

        if (subscribed && this->nodeConnected()) {
            // Send data back as received (parameters must be copied)
            std::thread t([this, topicExtended, payload]() {
                this->getNode()->receiveFar(topicExtended, payload);
            });
            t.detach();
        }

        return true;
    }

    bool LocalBroker::subscribe(const std::string& topic)
    {
        const std::scoped_lock lock(m_mutex);

        SPSP_LOGD("Subscribe to topic '%s'", topic.c_str());

        m_subs.insert(topic, true);
        return true;
    }

    bool LocalBroker::unsubscribe(const std::string& topic)
    {
        const std::scoped_lock lock(m_mutex);

        SPSP_LOGD("Unsubscribe from topic '%s'", topic.c_str());

        return m_subs.remove(topic);
    }
}
