/**
 * @file bridge.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Bridge node type of SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "spsp_nodes.hpp"
#include "wifi.hpp"

namespace SPSP::Nodes
{
    void Bridge::init()
    {
        WiFi& wifi = WiFi::instance();
        wifi.init();
    }

    void Bridge::deinit()
    {
        WiFi& wifi = WiFi::instance();
        wifi.deinit();
    }

    void Bridge::receiveLocal(Message msg)
    {
    }

    void Bridge::receiveFar(Message msg)
    {
    }
} // namespace SPSP
