/**
 * @file bridge.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Bridge node type of SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "spsp_logger.hpp"
#include "spsp_bridge.hpp"

// Log tag
static const char* SPSP_LOG_TAG = "SPSP/Bridge";

namespace SPSP::Nodes
{
    Bridge::Bridge()
    {
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
        // Unset old far layer
        if (m_fl != nullptr) this->unsetFarLayer();

        m_fl = fl;
        m_fl->setNode(this);
    }

    void Bridge::unsetFarLayer()
    {
        if (m_fl != nullptr) {
            m_fl->unsetNode();
            m_fl = nullptr;
        }
    }

    bool Bridge::receiveFar(const std::string topic, const std::string payload)
    {
        SPSP_LOGD("Received far msg: %s %s", topic.c_str(), payload.c_str());

        return true;
    }

    bool Bridge::publish(const std::string topic, const std::string payload)
    {
        SPSP_LOGD("Publishing locally: %s %s", topic.c_str(), payload.c_str());

        return m_fl->publish(topic, payload);
    }

    bool Bridge::subscribe(const std::string topic, SubscribeCb cb)
    {
        SPSP_LOGD("Subscribing locally to %s", topic.c_str());

        return this->subDBInsert(topic, LocalAddr{});
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
        return m_fl->publish(req.topic, req.payload);
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
                             uint8_t lifetime)
    {
        // Create sub entry
        BridgeSubEntry subEntry;
        subEntry.lifetime = lifetime;

        bool newTopic = m_subDB.find(topic) == m_subDB.end();

        // Subscribe
        if (newTopic) {
            bool subSuccess = m_fl->subscribe(topic);

            SPSP_LOGD("Subscribe to topic %s: %s", topic.c_str(),
                      subSuccess ? "success" : "fail");

            if (!subSuccess) return false;
        }

        // Add/update the entry
        m_subDB[topic][src] = subEntry;

        SPSP_LOGD("Sub DB: inserted entry: %s@%s (expires in %d min)",
                  (src.empty() ? src.str.c_str() : "."), topic.c_str(),
                  lifetime);

        return true;
    }

    bool Bridge::subDBRemove(const std::string topic, const LocalAddr src)
    {
        auto entry = m_subDB[topic][src];
        m_subDB[topic].erase(src);

        // Noone is subscribed to this topic anymore
        if (m_subDB[topic].size() == 0) {
            // Unsubscribe
            bool unsubSuccess = m_fl->unsubscribe(topic);

            SPSP_LOGD("Unsubscribe from topic %s: %s", topic.c_str(),
                      unsubSuccess ? "success" : "fail");

            if (!unsubSuccess) {
                // Restore original state
                m_subDB[topic][src] = entry;

                return false;
            }

            m_subDB.erase(topic);
        }

        return true;
    }

    void Bridge::subDBTick()
    {
        for (auto const& [topic, topicEntry] : m_subDB) {
            for (auto const& [src, subEntry] : topicEntry) {
                // Don't decrement entries with infinite lifetime
                if (subEntry.lifetime != BRIDGE_SUB_NO_EXPIRE) {
                    m_subDB[topic][src].lifetime--;
                }

                // Expired -> remove it
                if (subEntry.lifetime == 0) {
                    this->subDBRemove(topic, src);
                }
            }
        }
    }
} // namespace SPSP
