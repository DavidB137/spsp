/**
 * @file spsp_client_bridge.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Client-bridge node type of SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include "spsp_bridge.hpp"
#include "spsp_client.hpp"
#include "spsp_interfaces.hpp"

namespace SPSP::Nodes
{
    /**
     * @brief Client and bridge node
     * 
     */
    class ClientBridge : public Client, public Bridge
    {
    public:
        using Bridge::Bridge;

        /**
         * @brief Receives the message from local layer
         * 
         * Acts as a callback for local layer receiver.
         * 
         * @param msg Received message
         */
        void receiveLocal(const Message msg);

        using Bridge::receiveFar;
    };
} // namespace SPSP::Nodes
