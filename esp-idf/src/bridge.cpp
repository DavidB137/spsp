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

        // Add to subscribe database
        if (this->subDBInsert(req.topic, req.addr)) {
            // Subscribe
            bool delivered = m_fl->subscribe(req.topic);

            if (delivered) {
                SPSP_LOGD("Subscribed to topic %s", req.topic.c_str());
            } else {
                SPSP_LOGE("Subscribe to topic %s failed", req.topic.c_str());
            }

            return delivered;
        }

        return true;
    }

    bool Bridge::subDBInsert(const std::string topic, const LocalAddr src,
                             bool localLayer, uint8_t lifetime)
    {
        // Create sub entry
        BridgeSubEntry subEntry;
        subEntry.localLayer = localLayer;
        subEntry.lifetime = lifetime;

        bool newTopic = m_subDB.find(topic) == m_subDB.end();

        // Add/update the entry
        m_subDB[topic][src] = subEntry;

        SPSP_LOGD("Sub DB: inserted entry: %s@%s (expires in %d min)", 
                  (localLayer ? src.str.c_str() : "."), topic.c_str(),
                  lifetime);

        return newTopic;
    }

    void Bridge::subDBTick()
    {
        for (auto const& [topic, topicEntry] : m_subDB) {
            for (auto const& [src, subEntry] : topicEntry) {
                // Don't delete entries with lifetime UINT8_MAX
                if (subEntry.lifetime != UINT8_MAX) {
                    m_subDB[topic][src].lifetime--;
                }

                // Expired
                if (subEntry.lifetime == 0) {
                    // Remove it
                    m_subDB[topic].erase(src);

                    // Noone is subscribed to this topic anymore
                    if (m_subDB[topic].size() == 0) {
                        m_subDB.erase(topic);

                        // Unsubscribe
                        m_fl->unsubscribe(topic);
                    }
                }
            }
        }
    }
} // namespace SPSP
