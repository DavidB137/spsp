/**
 * @file client_sub_db.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Client subscribe database of SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "spsp_client_sub_db.hpp"
#include "spsp_logger.hpp"

// Log tag
static const char* SPSP_LOG_TAG = "SPSP/Client/SubDB";

namespace SPSP::Nodes
{
    // Wrapper for C timer callback
    void _client_sub_db_tick(TimerHandle_t xTimer)
    {
        ClientSubDB* subDB = static_cast<ClientSubDB*>(pvTimerGetTimerID(xTimer));

        // Create a new thread to prevent stack overflow
        std::thread t(&ClientSubDB::tick, subDB);
        t.detach();
    }

    ClientSubDB::ClientSubDB(Client* client) : m_client{client}, m_db{}
    {
        // Create timer for tick()
        m_timer = xTimerCreate("Client::subDBTick",
                               pdMS_TO_TICKS(60*1000),
                               pdTRUE, this,
                               _client_sub_db_tick);
        if (m_timer == nullptr) {
            SPSP_LOGE("Can't create FreeRTOS timer");
        } else if (xTimerStart(static_cast<TimerHandle_t>(m_timer), 0) == pdFAIL) {
            SPSP_LOGE("Can't start FreeRTOS timer");
        }

        SPSP_LOGD("Initialized");
    }

    ClientSubDB::~ClientSubDB()
    {
        // Delete timer
        auto timer = static_cast<TimerHandle_t>(m_timer);
        xTimerStop(timer, 0);
        xTimerDelete(timer, 0);

        SPSP_LOGD("Deinitialized");
    }

    void ClientSubDB::insert(const std::string topic, SPSP::SubscribeCb cb)
    {
        const std::lock_guard lock(m_mutex);

        m_db[topic] = {.cb = cb};

        SPSP_LOGD("Inserted topic '%s' with callback %p (renews in %d min)",
                  topic.c_str(), cb, m_db[topic].lifetime);
    }

    void ClientSubDB::remove(const std::string topic)
    {
        const std::lock_guard lock(m_mutex);

        m_db.erase(topic);

        SPSP_LOGD("Removed topic '%s'", topic.c_str());
    }

    void ClientSubDB::callCb(const std::string topic, const std::string payload)
    {
        m_mutex.lock();

        if (m_db.find(topic) != m_db.end()) {
            auto cb = m_db[topic].cb;

            m_mutex.unlock();

            SPSP_LOGD("Calling user callback (%p) for topic '%s'",
                      cb, topic.c_str());

            // Call user's callback
            cb(topic, payload);

            return;
        }

        m_mutex.unlock();

        SPSP_LOGD("No entry (callback) for topic '%s'", topic.c_str());
        return;
    }
    
    void ClientSubDB::tick()
    {
        m_mutex.lock();
        SPSP_LOGD("Tick running");

        for (auto const& [topic, entry] : m_db) {
            m_db[topic].lifetime--;

            // Expired -> renew it
            if (entry.lifetime == 0) {
                SPSP_LOGD("Topic '%s' expired (renewing)", topic.c_str());

                m_mutex.unlock();
                bool extended = m_client->subscribe(topic, entry.cb);
                m_mutex.lock();

                if (!extended) {
                    SPSP_LOGE("Topic '%s' can't be extended. Will try again in next tick.",
                              topic.c_str());

                    m_db[topic].lifetime++;
                }
            }
        }

        m_mutex.unlock();
        SPSP_LOGD("Tick done");
    }
} // namespace SPSP
