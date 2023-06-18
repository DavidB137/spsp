/**
 * @file bridge.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Bridge node type of SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "logger.hpp"
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

    bool Bridge::receiveLocal(const Message msg)
    {
        SPSP_LOGI("Received local msg: %s", msg.toString().c_str());

        bool delivered = true;

        // Call responsible handler
        switch (msg.type) {
        case MessageType::PING: delivered = processPing(msg); break;
        case MessageType::PONG: delivered = processPong(msg); break;
        case MessageType::PUB: delivered = processPub(msg); break;
        case MessageType::SUB_REQ: delivered = processSubReq(msg); break;
        case MessageType::SUB_DATA: delivered = processSubData(msg); break;
        default:
            SPSP_LOGW(
                "Unprocessable message type %s (%d)",
                messageTypeToStr(msg.type),
                static_cast<uint8_t>(msg.type)
            );
            break;
        }

        if (!delivered) {
            SPSP_LOGE("Message not delivered: %s", msg.toString().c_str());
        }

        return delivered;
    }

    bool Bridge::receiveFar(const std::string topic, const std::string payload)
    {
        SPSP_LOGI("Received far msg: %s %s", topic.c_str(), payload.c_str());

        return true;
    }

    bool Bridge::processPub(const Message req)
    {
        // Far layer is not connected - can't deliver
        if (!this->farLayerConnected()) return false;

        // Publish to far layer
        return m_fl->publish(req.topic, req.payload);
    }

    bool Bridge::processSubReq(const Message req)
    {
        // Far layer is not connected - can't deliver
        if (!this->farLayerConnected()) return false;

        // TODO
        // Add to subscribe database

        // Publish to far layer
        return m_fl->subscribe(req.topic);
    }
} // namespace SPSP
