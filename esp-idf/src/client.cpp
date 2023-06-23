/**
 * @file client.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Client node type of SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "spsp_client.hpp"
#include "spsp_logger.hpp"

// Log tag
static const char* SPSP_LOG_TAG = "SPSP/Client";

namespace SPSP::Nodes
{
    Client::Client()
    {
        SPSP_LOGI("Initialized");
    }

    Client::~Client()
    {
        SPSP_LOGI("Deinitialized");
    }

    bool Client::publish(const std::string topic, const std::string payload)
    {
        SPSP_LOGD("Publishing: %s %s", topic.c_str(), payload.c_str());
        return true;
    }

    bool Client::subscribe(const std::string topic, SubscribeCb cb)
    {
        SPSP_LOGD("Subscribing to %s", topic.c_str());
        return true;
    }

    bool Client::processProbeRes(const LocalMessage req)
    {
        return true;
    }

    bool Client::processSubData(const LocalMessage req)
    {
        return true;
    }
} // namespace SPSP
