/**
 * @file client.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Client node type of SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "spsp_client.hpp"
#include "spsp_logger.hpp"

// Log tag
static const char* SPSP_LOG_TAG = "SPSP/Client";

namespace SPSP::Nodes
{
    // Wrapper for C timer callback
    void _client_subdb_tick(TimerHandle_t xTimer)
    {
        Client* c = static_cast<Client*>(pvTimerGetTimerID(xTimer));
        c->subDBTick();
    }

    Client::Client()
    {
        // Create timer for subDBTick()
        TimerHandle_t subDBTickTimer = xTimerCreate("Client::subDBTick",
                                                    pdMS_TO_TICKS(60*1000),
                                                    pdTRUE, this,
                                                    _client_subdb_tick);
        if (subDBTickTimer == nullptr) {
            SPSP_LOGE("Can't create FreeRTOS timer");
        } else if (xTimerStart(subDBTickTimer, 0) == pdFAIL) {
            SPSP_LOGE("Can't start FreeRTOS timer");
        }

        SPSP_LOGI("Initialized");
    }

    Client::~Client()
    {
        SPSP_LOGI("Deinitialized");
    }

    bool Client::publish(const std::string topic, const std::string payload)
    {
        SPSP_LOGD("Publishing: %s %s", topic.c_str(), payload.c_str());

        LocalMessage msg = {};
        // msg.addr is empty => send to the bridge node
        msg.type = LocalMessageType::PUB;
        msg.topic = topic;
        msg.payload = payload;

        return this->sendLocal(msg);
    }

    bool Client::subscribe(const std::string topic, SubscribeCb cb)
    {
        SPSP_LOGD("Subscribing to %s", topic.c_str());

        LocalMessage msg = {};
        // msg.addr is empty => send to the bridge node
        msg.type = LocalMessageType::SUB_REQ;
        msg.topic = topic;
        // msg.payload is empty

        // Add to sub DB
        m_mutex.lock();
        m_subDB[topic] = {.cb = cb};
        m_mutex.unlock();

        return this->sendLocal(msg);
    }

    bool Client::processSubData(const LocalMessage req)
    {
        m_mutex.lock();

        if (m_subDB.find(req.topic) != m_subDB.end()) {
            auto cb = m_subDB[req.topic].cb;

            SPSP_LOGD("SUB DATA: calling user callback (%p) for topic %s",
                      cb, req.topic.c_str());

            m_mutex.unlock();

            // Call user's callback
            cb(req.topic, req.payload);
            return true;
        }

        m_mutex.unlock();
        return false;
    }

    void Client::subDBTick()
    {
        const std::lock_guard lock(m_mutex);

        SPSP_LOGD("Sub DB: tick running");

        for (auto const& [topic, subEntry] : m_subDB) {
            m_subDB[topic].lifetime--;

            // Expired -> remove it
            if (subEntry.lifetime == 0) {
                SPSP_LOGD("Sub DB: topic %s expired", topic.c_str());
                // TODO: renew it and add unsubscribe() method
                // and use erase_if() C++20
                //m_subDB.erase(topic);
            }
        }
    }
} // namespace SPSP
