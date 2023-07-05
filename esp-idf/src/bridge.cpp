/**
 * @file bridge.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Bridge node type of SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "spsp_bridge.hpp"
#include "spsp_logger.hpp"

// Log tag
static const char* SPSP_LOG_TAG = "SPSP/Bridge";

namespace SPSP::Nodes
{
    // Wrapper for C timer callback
    void _bridge_subdb_tick(TimerHandle_t xTimer)
    {
        Bridge* b = static_cast<Bridge*>(pvTimerGetTimerID(xTimer));
        b->subDBTick();
    }

    Bridge::Bridge()
    {
        // Create timer for subDBTick()
        TimerHandle_t subDBTickTimer = xTimerCreate("Bridge::subDBTick",
                                                    pdMS_TO_TICKS(60*1000),
                                                    pdTRUE, this,
                                                    _bridge_subdb_tick);
        if (subDBTickTimer == nullptr) {
            SPSP_LOGE("Can't create FreeRTOS timer");
        } else if (xTimerStart(subDBTickTimer, 0) == pdFAIL) {
            SPSP_LOGE("Can't start FreeRTOS timer");
        }

        SPSP_LOGI("Initialized");
    }

    Bridge::~Bridge()
    {
        // Unset far layer (if not already unset)
        this->unsetFarLayer();

        SPSP_LOGI("Deinitialized");
    }

    void Bridge::setFarLayer(IFarLayer* fl)
    {
        const std::lock_guard lock(m_mutex);

        // Unset old far layer
        if (m_fl != nullptr) this->unsetFarLayer();

        m_fl = fl;
        m_fl->setNode(this);

        SPSP_LOGD("Set far layer");
    }

    void Bridge::unsetFarLayer()
    {
        const std::lock_guard lock(m_mutex);

        if (m_fl != nullptr) {
            m_fl->unsetNode();
            m_fl = nullptr;
        }

        SPSP_LOGD("Unset far layer");
    }

    bool Bridge::receiveFar(const std::string topic, const std::string payload)
    {
        SPSP_LOGD("Received far msg: %s %s", topic.c_str(), payload.c_str());

        return true;
    }

    bool Bridge::publish(const std::string topic, const std::string payload)
    {
        SPSP_LOGD("Publishing locally: %s %s", topic.c_str(), payload.c_str());

        if (!this->farLayerConnected()) {
            SPSP_LOGE("Publish: far layer is not connected");
            return false;
        }

        return m_fl->publish(LocalAddr{}.str, topic, payload);
    }

    bool Bridge::subscribe(const std::string topic, SubscribeCb cb)
    {
        SPSP_LOGD("Subscribing locally to %s", topic.c_str());

        return this->subDBInsert(topic, LocalAddr{}, BRIDGE_SUB_NO_EXPIRE, cb);
    }

    bool Bridge::unsubscribe(const std::string topic)
    {
        // TODO
        return true;
    }

    bool Bridge::processProbeReq(const LocalMessage req)
    {
        LocalMessage res = req;
        res.type = LocalMessageType::PROBE_RES;
        return this->sendLocal(res);
    }

    bool Bridge::processPub(const LocalMessage req)
    {
        // Far layer is not connected - can't deliver
        if (!this->farLayerConnected()) {
            SPSP_LOGE("Can't publish - far layer is not connected");
            return false;
        }

        // Publish to far layer
        return m_fl->publish(req.addr.str, req.topic, req.payload);
    }

    bool Bridge::processSubReq(const LocalMessage req)
    {
        // Far layer is not connected - can't deliver
        if (!this->farLayerConnected()) {
            SPSP_LOGE("Can't publish - far layer is not connected");
            return false;
        }

        return this->subDBInsert(req.topic, req.addr);
    }

    bool Bridge::subDBInsert(const std::string topic, const LocalAddr src,
                             uint8_t lifetime, SubscribeCb cb)
    {
        m_mutex.lock();

        // Create sub entry
        BridgeSubEntry subEntry = {};
        subEntry.lifetime = lifetime;
        subEntry.cb = cb;

        bool newTopic = m_subDB.find(topic) == m_subDB.end();

        // Subscribe
        if (newTopic) {
            if (!this->farLayerConnected()) {
                SPSP_LOGE("Sub DB: far layer is not connected");
                m_mutex.unlock();
                return false;
            }

            m_mutex.unlock();

            bool subSuccess = m_fl->subscribe(topic);

            SPSP_LOGD("Subscribe to topic %s: %s", topic.c_str(),
                      subSuccess ? "success" : "fail");

            if (!subSuccess) return false;

            m_mutex.lock();
        }

        // Add/update the entry
        m_subDB[topic][src] = subEntry;

        m_mutex.unlock();

        SPSP_LOGD("Sub DB: inserted entry: %s@%s (expires in %d min)",
                  (src.empty() ? "." : src.str.c_str()), topic.c_str(),
                  lifetime);

        return true;
    }

    void Bridge::subDBTick()
    {
        SPSP_LOGD("Sub DB: tick running");

        // Decrement lifetimes
        this->subDBDecrementLifetimes();

        // Remove expired sub entries
        this->subDBRemoveExpiredSubEntries();

        // Unsubscribe from unused topics
        this->subDBRemoveUnusedTopics();
    }

    void Bridge::subDBDecrementLifetimes()
    {
        const std::lock_guard lock(m_mutex);

        for (auto const& [topic, topicEntry] : m_subDB) {
            for (auto const& [src, subEntry] : topicEntry) {
                // Don't decrement entries with infinite lifetime
                if (subEntry.lifetime != BRIDGE_SUB_NO_EXPIRE) {
                    m_subDB[topic][src].lifetime--;
                }
            }
        }
    }

    void Bridge::subDBRemoveExpiredSubEntries()
    {
        const std::lock_guard lock(m_mutex);

        for (auto const& [topic, topicEntry] : m_subDB) {
            auto subEntryIt = topicEntry.begin();
            while (subEntryIt != topicEntry.end()) {
                auto src = subEntryIt->first;

                if (subEntryIt->second.lifetime == 0) {
                    SPSP_LOGD("Sub DB: remove src %s from topic %s",
                             src.str.c_str(), topic.c_str());
                    subEntryIt = m_subDB[topic].erase(subEntryIt);
                } else {
                    subEntryIt++;
                }
            }
        }
    }

    void Bridge::subDBRemoveUnusedTopics()
    {
        m_mutex.lock();

        auto topicEntryIt = m_subDB.begin();
        while (topicEntryIt != m_subDB.end()) {
            auto topic = topicEntryIt->first;

            // If unused
            if (topicEntryIt->second.empty()) {
                if (!this->farLayerConnected()) {
                    SPSP_LOGE("Sub DB: far layer is not connected");
                    m_mutex.unlock();
                    return;
                }

                m_mutex.unlock();

                // Unsubscribe
                bool unsubSuccess = m_fl->unsubscribe(topic);

                SPSP_LOGD("Sub DB: unsubscribe from topic %s: %s",
                          topic.c_str(), unsubSuccess ? "success" : "fail");

                if (!unsubSuccess) return;

                m_mutex.lock();

                SPSP_LOGD("Sub DB: remove topic %s", topic.c_str());

                topicEntryIt = m_subDB.erase(topicEntryIt);
            } else {
                topicEntryIt++;
            }
        }

        m_mutex.unlock();
    }
} // namespace SPSP
