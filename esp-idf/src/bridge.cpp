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

    void Bridge::receiveLocal(const Message msg)
    {
        SPSP_LOGI("Received local msg: %s", msg.toString().c_str());
    }

    void Bridge::receiveFar(const std::string topic, const std::string payload)
    {
        SPSP_LOGI("Received far msg: %s %s", topic.c_str(), payload.c_str());
    }

    bool Bridge::sendLocal(const Message msg)
    {
        return true;  // TODO
    }

    bool Bridge::processPong(const Message req)
    {
        return true;  // TODO
    }

    bool Bridge::processPub(const Message req)
    {
        return true;  // TODO
    }

    bool Bridge::processSubReq(const Message req)
    {
        return true;  // TODO
    }

    bool Bridge::processSubData(const Message req)
    {
        return true;  // TODO
    }
} // namespace SPSP
