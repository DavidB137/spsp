/**
 * @file bridge.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Bridge node type of SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "logger.hpp"
#include "spsp_nodes.hpp"

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

    void Bridge::receiveLocal(Message msg)
    {
        SPSP_LOGI("Received local msg: %s", msg.toString().c_str());
    }

    void Bridge::receiveFar(Message msg)
    {
        SPSP_LOGI("Received far msg: %s", msg.toString().c_str());
    }
} // namespace SPSP
