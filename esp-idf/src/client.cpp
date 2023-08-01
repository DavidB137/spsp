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
#include "spsp_version.hpp"

// Log tag
static const char* SPSP_LOG_TAG = "SPSP/Client";

namespace SPSP::Nodes
{
    Client::Client() : m_subDB{this}
    {
        SPSP_LOGI("SPSP version: %s", SPSP::VERSION);
        SPSP_LOGI("Initialized");
    }

    Client::~Client()
    {
        SPSP_LOGI("Deinitialized");
    }

    bool Client::publish(const std::string topic, const std::string payload)
    {
        SPSP_LOGD("Publishing: topic '%s', payload '%s'",
                  topic.c_str(), payload.c_str());

        LocalMessage<LocalAddr> msg = {};
        // msg.addr is empty => send to the bridge node
        msg.type = LocalMessageType::PUB;
        msg.topic = topic;
        msg.payload = payload;

        return this->sendLocal(msg);
    }

    bool Client::subscribe(const std::string topic, SubscribeCb cb)
    {
        SPSP_LOGD("Subscribing to topic '%s'", topic.c_str());

        LocalMessage<LocalAddr> msg = {};
        // msg.addr is empty => send to the bridge node
        msg.type = LocalMessageType::SUB_REQ;
        msg.topic = topic;
        // msg.payload is empty

        // Add to sub DB
        m_subDB.insert(topic, cb);

        return this->sendLocal(msg);
    }

    bool Client::unsubscribe(const std::string topic)
    {
        SPSP_LOGD("Unsubscribing from topic '%s'", topic.c_str());

        LocalMessage<LocalAddr> msg = {};
        // msg.addr is empty => send to the bridge node
        msg.type = LocalMessageType::UNSUB;
        msg.topic = topic;
        // msg.payload is empty

        // Remove from sub DB
        m_subDB.remove(topic);

        return this->sendLocal(msg);
    }

    bool Client::processSubData(const LocalMessage<LocalAddr> req)
    {
        m_subDB.callCb(req.topic, req.payload);
        return true;
    }
} // namespace SPSP
