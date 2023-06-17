/**
 * @file spsp.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Simple publish-subscribe protocol
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <cassert>

#include "spsp_message.hpp"

namespace SPSP
{
    // Forward declaration
    class INode;

    /**
     * @brief Generic interface for local or far layer
     * 
     * Has a pointer to owner node (to directly call `Node::receive()` method). 
     */
    class ILocalOrFarLayer
    {
        friend class INode;

    private:
        INode* node;

    public:
        /**
         * @brief Constructs a new layer object
         * 
         */
        ILocalOrFarLayer() : node{nullptr} {};

    protected:
        /**
         * @brief Sets the pointer to the owner node.
         * 
         * @param n Owner node
         */
        void setNode(INode* n) { node = n; };

        /**
         * @brief Gets the node object
         * 
         * Asserts the node is not NULL.
         * 
         * @return Node pointer
         */
        INode* getNode()
        {
            assert(node != nullptr);
            return node;
        }
    };

    /**
     * @brief Interface for local layer
     * 
     */
    class ILocalLayer : public ILocalOrFarLayer
    {
    protected:
        /**
         * @brief Sends the message to given node
         * 
         * @param msg Message
         */
        virtual void send(Message msg) = 0;
    };

    /**
     * @brief Interface for far layer
     * 
     */
    class IFarLayer : public ILocalOrFarLayer
    {
    protected:
        /**
         * @brief Publishes message coming from node
         * 
         * @param topic Topic
         * @param payload Payload (data)
         */
        virtual void publish(std::string topic, std::string payload) = 0;
    };

    /**
     * @brief Generic node of SPSP
     * 
     * Implements common functionality for client and bridge node types.
     */
    class INode
    {
        friend class ILocalOrFarLayer;

    protected:
        ILocalLayer& ll;
        IFarLayer& fl;

    public:
        /**
         * @brief Constructs a new Node object
         * 
         * @param ll Local layer
         * @param fl Far layer
         */
        INode(ILocalLayer& ll, IFarLayer& fl) : ll{ll}, fl{fl}
        {
            ll.setNode(this);
            fl.setNode(this);
        };

        /**
         * @brief Initializes the node
         * 
         */
        virtual void init() = 0;

        /**
         * @brief Deinitializes the node
         * 
         */
        virtual void deinit() = 0;
    
    protected:
        /**
         * @brief Receives the message from local layer
         * 
         * Acts as a callback for local layer receiver.
         * 
         * @param msg Received message
         */
        virtual void receiveLocal(Message msg) = 0;

        /**
         * @brief Receives the message from far layer
         * 
         * Acts as a callback for far layer receiver.
         * 
         * @param msg Received message
         */
        virtual void receiveFar(Message msg) = 0;
    };
} // namespace SPSP
