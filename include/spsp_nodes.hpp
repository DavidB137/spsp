/**
 * @file spsp_nodes.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Node types of SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef SPSP_NODES_HPP
#define SPSP_NODES_HPP

#include "spsp.hpp"

namespace SPSP::Nodes
{
    /**
     * @brief Client node
     * 
     */
    class Client : public SPSP::Node
    {
    public:
        using SPSP::Node::Node;

        /**
         * @brief Initializes client node
         * 
         */
        void init();

    protected:
        /**
         * @brief Receives the message from local layer
         * 
         * Acts as a callback for local layer receiver.
         * 
         * @param msg Received message
         * @return Message Reply that should be sent back
         */
        Message receive(Message msg);
    };

    /**
     * @brief Bridge node
     * 
     */
    class Bridge : public SPSP::Node
    {
    public:
        using SPSP::Node::Node;

        /**
         * @brief Initializes bridge node
         * 
         */
        void init();

    protected:
        /**
         * @brief Receives the message from local layer
         * 
         * Acts as a callback for local layer receiver.
         * 
         * @param msg Received message
         * @return Message Reply that should be sent back
         */
        Message receive(Message msg);
    };

    /**
     * @brief Client and bridge node
     * 
     */
    class ClientBridge : public Client, public Bridge
    {
    public:
        using Bridge::Bridge;

        /**
         * @brief Initializes client-bridge node
         * 
         */
        void init();
    
    protected:
        /**
         * @brief Receives the message from local layer
         * 
         * Acts as a callback for local layer receiver.
         * 
         * @param msg Received message
         * @return Message Reply that should be sent back
         */
        Message receive(Message msg);
    };
} // namespace SPSP::Nodes

#endif
