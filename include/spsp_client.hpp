/**
 * @file spsp_client.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Client node type of SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include "spsp_interfaces.hpp"

namespace SPSP::Nodes
{
    /**
     * @brief Client node
     * 
     */
    class Client : public SPSP::INode
    {
    public:
        using SPSP::INode::INode;

        /**
         * @brief Receives the message from local layer
         * 
         * Acts as a callback for local layer receiver.
         * 
         * @param msg Received message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool receiveLocal(Message msg);
    };
} // namespace SPSP::Nodes
