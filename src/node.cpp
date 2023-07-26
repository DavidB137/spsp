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

    void INode::receiveLocal(const LocalMessage msg, int rssi)
    {
        if (rssi < INT_MAX) {
            SPSP_LOGI("Received local msg: %s (%d dBm)", msg.toString().c_str(), rssi);
        } else {
            SPSP_LOGI("Received local msg: %s", msg.toString().c_str());
        }

        // Call receive/send callback
        if (m_localRecvSendCb != nullptr) {
            SPSP_LOGD("Calling receive/send callback %p", m_localRecvSendCb);
            m_localRecvSendCb(msg);
        }

        bool processed = false;

        // Call responsible handler
        switch (msg.type) {
        case LocalMessageType::PROBE_REQ: processed = processProbeReq(msg); break;
        case LocalMessageType::PROBE_RES: processed = processProbeRes(msg); break;
        case LocalMessageType::PUB: processed = processPub(msg); break;
        case LocalMessageType::SUB_REQ: processed = processSubReq(msg); break;
        case LocalMessageType::SUB_DATA: processed = processSubData(msg); break;
        case LocalMessageType::UNSUB: processed = processUnsub(msg); break;
        default:
            SPSP_LOGW("Unprocessable message type %s (%d)",
                      localMessageTypeToStr(msg.type),
                      static_cast<uint8_t>(msg.type));
            break;
        }

        if (processed) {
            SPSP_LOGI("Message processed: %s", msg.toString().c_str());
        } else {
            SPSP_LOGE("Message not processed: %s", msg.toString().c_str());
        }
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

        if (delivered) {
            SPSP_LOGI("Message delivered: %s", msg.toString().c_str());
        } else {
            SPSP_LOGE("Message not delivered: %s", msg.toString().c_str());
        }

        // Call receive/send callback
        if (m_localRecvSendCb != nullptr) {
            SPSP_LOGD("Calling receive/send callback %p", m_localRecvSendCb);
            m_localRecvSendCb(msg);
        }

        return delivered;
    }
} // namespace SPSP
