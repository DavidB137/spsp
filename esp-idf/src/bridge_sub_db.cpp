/**
 * @file bridge_sub_db.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Bridge subscribe database of SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "spsp_bridge.hpp"
#include "spsp_logger.hpp"

// Log tag
static const char* SPSP_LOG_TAG = "SPSP/Bridge/SubDB";

namespace SPSP::Nodes
{
    // Wrapper for C timer callback
    void _bridge_sub_db_tick(TimerHandle_t xTimer)
    {
        BridgeSubDB* subDB = static_cast<BridgeSubDB*>(pvTimerGetTimerID(xTimer));

        // Create a new thread to prevent stack overflow
        std::thread t(&BridgeSubDB::tick, subDB);
        t.detach();
    }

    BridgeSubDB::BridgeSubDB(Bridge* bridge) : m_bridge{bridge}, m_db{}
    {
        // Create timer for tick()
        m_timer = xTimerCreate("Bridge::subDBTick",
                               pdMS_TO_TICKS(60*1000),
                               pdTRUE, this,
                               _bridge_sub_db_tick);
        if (m_timer == nullptr) {
            SPSP_LOGE("Can't create FreeRTOS timer");
        } else if (xTimerStart(static_cast<TimerHandle_t>(m_timer), 0) == pdFAIL) {
            SPSP_LOGE("Can't start FreeRTOS timer");
        }

        SPSP_LOGD("Initialized");
    }

    BridgeSubDB::~BridgeSubDB()
    {
        // Delete timer
        auto timer = static_cast<TimerHandle_t>(m_timer);
        xTimerStop(timer, 0);
        xTimerDelete(timer, 0);

        SPSP_LOGD("Deinitialized");
    }

    bool BridgeSubDB::insert(const std::string topic, const LocalAddr addr,
                             const SubscribeCb cb)
    {
        const std::lock_guard lock(m_mutex);

        // Create sub entry
        Entry subEntry = {};
        if (addr.empty()) {
            subEntry.lifetime = BRIDGE_SUB_NO_EXPIRE;
            subEntry.cb = cb;
        }

        bool newTopic = m_db.find(topic) == m_db.end();

        if (newTopic) {
            // Subscribe to the topic
            if (!m_bridge->subscribeFar(topic)) {
                SPSP_LOGE("Insert: subscribe to topic %s failed - not inserting anything",
                          topic.c_str());
                return false;
            }
        }

        // Add/update the entry
        m_db[topic][addr] = subEntry;

        if (addr.empty()) {
            SPSP_LOGD("Inserted local entry for topic %s with callback %p (no expiration)",
                      topic.c_str(), subEntry.cb);
        } else {
            SPSP_LOGD("Inserted %s@%s (expires in %d min)",
                      addr.str.c_str(), topic.c_str(), subEntry.lifetime);
        }

        return true;
    }

    void BridgeSubDB::remove(const std::string topic, const LocalAddr addr)
    {
        const std::lock_guard lock(m_mutex);

        // Topic doesn't exist
        if (m_db.find(topic) == m_db.end()) return;

        m_db[topic].erase(addr);

        SPSP_LOGD("Removed addr %s on topic %s",
                  addr.empty() ? "." : addr.str.c_str(), topic.c_str());

        // Unsubscribe if not needed anymore
        this->removeUnusedTopics();
    }

    void BridgeSubDB::resubscribeAll()
    {
        const std::lock_guard lock(m_mutex);

        for (auto const& [topic, topicEntry] : m_db) {
            if (!m_bridge->subscribeFar(topic)) {
                SPSP_LOGW("Resubscribe to topic %s failed", topic.c_str());
            }
        }

        SPSP_LOGD("Resubscribed to %d topics", m_db.size());
    }

    void BridgeSubDB::callCbs(const std::string topic, const std::string payload)
    {
        m_mutex.lock();

        if (auto topicIt = m_db.find(topic); topicIt != m_db.end()) {
            for (auto const& [addr, subEntry] : topicIt->second) {
                m_mutex.unlock();

                if (addr.empty()) {
                    // This node's subscription - call callback
                    SPSP_LOGD("Calling user callback (%p) for topic %s",
                              subEntry.cb, topic.c_str());
                    subEntry.cb(topic, payload);
                } else {
                    // Local layer subscription
                    m_bridge->publishSubData(addr, topic, payload);
                }

                m_mutex.lock();
            }
        }

        m_mutex.unlock();
    }
    
    void BridgeSubDB::tick()
    {
        SPSP_LOGD("Tick running");

        this->decrementLifetimes();
        this->removeExpiredSubEntries();
        this->removeUnusedTopics();

        SPSP_LOGD("Tick done");
    }

    void BridgeSubDB::decrementLifetimes()
    {
        const std::lock_guard lock(m_mutex);

        for (auto const& [topic, topicEntry] : m_db) {
            for (auto const& [addr, subEntry] : topicEntry) {
                // Don't decrement entries with infinite lifetime
                if (subEntry.lifetime != BRIDGE_SUB_NO_EXPIRE) {
                    m_db[topic][addr].lifetime--;
                }
            }
        }
    }

    void BridgeSubDB::removeExpiredSubEntries()
    {
        const std::lock_guard lock(m_mutex);

        for (auto const& [topic, topicEntry] : m_db) {
            auto subEntryIt = topicEntry.begin();
            while (subEntryIt != topicEntry.end()) {
                auto src = subEntryIt->first;

                if (subEntryIt->second.lifetime == 0) {
                    // Expired
                    subEntryIt = m_db[topic].erase(subEntryIt);
                    SPSP_LOGD("Removed src %s from topic %s",
                              src.str.c_str(), topic.c_str());
                } else {
                    // Continue
                    subEntryIt++;
                }
            }
        }
    }

    void BridgeSubDB::removeUnusedTopics()
    {
        const std::lock_guard lock(m_mutex);

        auto topicEntryIt = m_db.begin();
        while (topicEntryIt != m_db.end()) {
            auto topic = topicEntryIt->first;

            // If unused
            if (topicEntryIt->second.empty()) {
                bool unsubSuccess = m_bridge->unsubscribeFar(topic);

                if (unsubSuccess) {
                    topicEntryIt = m_db.erase(topicEntryIt);
                    SPSP_LOGD("Removed unused topic %s", topic.c_str());
                } else {
                    SPSP_LOGE("Topic %s can't be unsubscribed. Will try again in next tick.",
                              topic.c_str());
                }
            } else {
                topicEntryIt++;
            }
        }
    }
} // namespace SPSP
