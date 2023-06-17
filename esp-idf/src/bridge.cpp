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
#include "wifi.hpp"

// Log tag
static const char* SPSP_LOG_TAG = "SPSP/Bridge";

namespace SPSP::Nodes
{
    Bridge::Bridge(SPSP::ILocalLayer* ll, SPSP::IFarLayer* fl) : SPSP::INode{ll}, fl{fl}
    {
        // WiFi
        WiFi& wifi = WiFi::instance();
        wifi.init();

        // Layers
        ll->setNode(this);
        fl->setNode(this);

        SPSP_LOGI("Initialized");
    }

    Bridge::~Bridge()
    {
        // Layers
        ll->setNode(nullptr);
        fl->setNode(nullptr);

        // WiFi
        WiFi& wifi = WiFi::instance();
        wifi.deinit();

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
