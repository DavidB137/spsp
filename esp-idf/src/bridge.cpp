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
#include "spsp_version.hpp"

// Log tag
static const char* SPSP_LOG_TAG = "SPSP/Bridge";

namespace SPSP::Nodes
{
    Bridge::Bridge() : m_subDB{this}
    {
        SPSP_LOGI("SPSP version: %s", SPSP::VERSION);
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

        // Subscribe to all topics
        m_subDB.resubscribeAll();
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
        SPSP_LOGD("Received far msg: topic '%s', payload '%s'",
                  topic.c_str(), payload.c_str());

        m_subDB.callCbs(topic, payload);
        return true;
    }

    bool Bridge::publish(const std::string topic, const std::string payload)
    {
        SPSP_LOGD("Publishing locally: topic '%s', payload '%s'",
                  topic.c_str(), payload.c_str());

        if (!this->farLayerConnected()) {
            SPSP_LOGE("Publish: far layer is not connected");
            return false;
        }

        return m_fl->publish(LocalAddr{}.str, topic, payload);
    }

    bool Bridge::subscribe(const std::string topic, SubscribeCb cb)
    {
        SPSP_LOGD("Subscribing locally to topic '%s'", topic.c_str());

        return m_subDB.insert(topic, LocalAddr{}, cb);
    }

    bool Bridge::unsubscribe(const std::string topic)
    {
        SPSP_LOGD("Unsubscribing locally from topic '%s'", topic.c_str());

        m_subDB.remove(topic, LocalAddr{});
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
        return m_subDB.insert(req.topic, req.addr);
    }

    bool Bridge::processUnsub(const LocalMessage req)
    {
        m_subDB.remove(req.topic, req.addr);
        return true;
    }

    bool Bridge::publishSubData(const LocalAddr addr, const std::string topic,
                                const std::string payload)
    {
        SPSP_LOGD("Sending SUB_DATA to %s: topic '%s', payload '%s'",
                  addr.str.c_str(), topic.c_str(), payload.c_str());

        LocalMessage msg = {};
        msg.addr = addr;
        msg.type = LocalMessageType::SUB_DATA;
        msg.topic = topic;
        msg.payload = payload;

        return this->sendLocal(msg);
    }

    bool Bridge::subscribeFar(const std::string topic)
    {
        // Far layer is not connected - can't deliver
        if (!this->farLayerConnected()) {
            SPSP_LOGE("Can't subscribe - far layer is not connected");
            return false;
        }

        return m_fl->subscribe(topic);
    }

    bool Bridge::unsubscribeFar(const std::string topic)
    {
        // Far layer is not connected - can't deliver
        if (!this->farLayerConnected()) {
            SPSP_LOGE("Can't unsubscribe - far layer is not connected");
            return false;
        }

        return m_fl->unsubscribe(topic);
    }
} // namespace SPSP
