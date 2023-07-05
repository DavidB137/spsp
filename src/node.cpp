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
        const std::lock_guard lock(m_mutex);

        // Unset old local layer
        if (m_ll != nullptr) this->unsetLocalLayer();

        m_ll = ll;
        m_ll->setNode(this);

        SPSP_LOGD("Set far layer");
    }

    void INode::unsetLocalLayer()
    {
        const std::lock_guard lock(m_mutex);

        if (m_ll != nullptr) {
            m_ll->unsetNode();
            m_ll = nullptr;
        }

        SPSP_LOGD("Unset far layer");
    }

    bool INode::receiveLocal(const LocalMessage msg, int rssi)
    {
        if (rssi < INT_MAX) {
            SPSP_LOGI("Received local msg: %s (%d dBm)", msg.toString().c_str(), rssi);
        } else {
            SPSP_LOGI("Received local msg: %s", msg.toString().c_str());
        }

        bool delivered = true;

        // Call responsible handler
        switch (msg.type) {
        case LocalMessageType::PROBE_REQ: delivered = processProbeReq(msg); break;
        case LocalMessageType::PROBE_RES: delivered = processProbeRes(msg); break;
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
            SPSP_LOGE("Message not processed: %s", msg.toString().c_str());
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

        SPSP_LOGI("Sending local msg: %s", msg.toString().c_str());

        // Send to local layer
        bool delivered = m_ll->send(msg);

        SPSP_LOGI("Message %s: %s", delivered ? "delivered" : "not delivered",
                  msg.toString().c_str());

        return delivered;
    }
} // namespace SPSP
