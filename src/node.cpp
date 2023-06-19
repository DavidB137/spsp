/**
 * @file node.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Node interface for SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "spsp_logger.hpp"
#include "spsp_node.hpp"

// Log tag
static const char* SPSP_LOG_TAG = "SPSP/Node";

namespace SPSP
{
    void INode::setLocalLayer(ILocalLayer* ll)
    {
        // Unset old local layer
        if (m_ll != nullptr) this->unsetLocalLayer();

        m_ll = ll;
        m_ll->setNode(this);
    }

    void INode::unsetLocalLayer()
    {
        if (m_ll != nullptr) {
            m_ll->unsetNode();
            m_ll = nullptr;
        }
    }

    bool INode::receiveLocal(const LocalMessage msg, int rssi)
    {
        if (rssi < INT_MAX) {
            SPSP_LOGD("Received local msg: %s (%d dBm)", msg.toString().c_str(), rssi);
        } else {
            SPSP_LOGD("Received local msg: %s", msg.toString().c_str());
        }

        bool delivered = true;

        // Call responsible handler
        switch (msg.type) {
        case LocalMessageType::PING: delivered = processPing(msg); break;
        case LocalMessageType::PONG: delivered = processPong(msg); break;
        case LocalMessageType::PUB: delivered = processPub(msg); break;
        case LocalMessageType::SUB_REQ: delivered = processSubReq(msg); break;
        case LocalMessageType::SUB_DATA: delivered = processSubData(msg); break;
        default:
            SPSP_LOGW("Unprocessable message type %s (%d)",
                      localMessageTypeToStr(msg.type),
                      static_cast<uint8_t>(msg.type));
            break;
        }

        if (!delivered) {
            SPSP_LOGE("Message not delivered: %s", msg.toString().c_str());
        }

        return delivered;
    }

    bool INode::sendLocal(const LocalMessage msg)
    {
        // Local layer is not connected - can't deliver
        if (!this->localLayerConnected()) {
            SPSP_LOGE("Can't send to local layer - local layer is not connected");
            return false;
        }

        // Send to local layer
        return m_ll->send(msg);
    }

    bool INode::processPing(const LocalMessage req)
    {
        LocalMessage res = req;
        res.type = LocalMessageType::PONG;
        return this->sendLocal(res);
    }
} // namespace SPSP
