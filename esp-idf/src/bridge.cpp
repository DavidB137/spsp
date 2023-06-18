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
