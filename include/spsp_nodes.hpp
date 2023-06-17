/**
 * @file spsp_nodes.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Node types of SPSP
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
         * @brief Initializes client node
         * 
         */
        void init();

        /**
         * @brief Deinitializes client node
         * 
         */
        void deinit();

    protected:
        /**
         * @brief Receives the message from local layer
         * 
         * Acts as a callback for local layer receiver.
         * 
         * @param msg Received message
         */
        void receiveLocal(Message msg);

        /**
         * @brief Receives the message from far layer
         * 
         * Acts as a callback for far layer receiver.
         * 
         * @param msg Received message
         */
        void receiveFar(Message msg);
    };

    /**
     * @brief Bridge node
     * 
     */
    class Bridge : public SPSP::INode
    {
    public:
        using SPSP::INode::INode;

        /**
         * @brief Initializes bridge node
         * 
         */
        void init();

        /**
         * @brief Deinitializes bridge node
         * 
         */
        void deinit();

    protected:
        /**
         * @brief Receives the message from local layer
         * 
         * Acts as a callback for local layer receiver.
         * 
         * @param msg Received message
         */
        void receiveLocal(Message msg);

        /**
         * @brief Receives the message from far layer
         * 
         * Acts as a callback for far layer receiver.
         * 
         * @param msg Received message
         */
        void receiveFar(Message msg);
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

        /**
         * @brief Deinitializes client-bridge node
         * 
         */
        void deinit();
    
    protected:
        /**
         * @brief Receives the message from local layer
         * 
         * Acts as a callback for local layer receiver.
         * 
         * @param msg Received message
         */
        void receiveLocal(Message msg);

        /**
         * @brief Receives the message from far layer
         * 
         * Acts as a callback for far layer receiver.
         * 
         * @param msg Received message
         */
        void receiveFar(Message msg);
    };
} // namespace SPSP::Nodes
