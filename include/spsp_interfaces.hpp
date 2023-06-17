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
     * Has a pointer to owner node (to directly call `INode::receive()` method). 
     */
    class ILocalOrFarLayer
    {
    private:
        INode* node;

    public:
        /**
         * @brief Constructs a new layer object
         * 
         */
        ILocalOrFarLayer() : node{nullptr} {};

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
    public:
        /**
         * @brief Sends the message to given node
         * 
         * Should be used by `INode` only!
         * 
         * @param msg Message
         * @return true Delivery successful
         * @return false Delivery failed
         */
        virtual bool send(Message msg) = 0;
    };

    /**
     * @brief Interface for far layer
     * 
     */
    class IFarLayer : public ILocalOrFarLayer
    {
    public:
        /**
         * @brief Publishes message coming from node
         * 
         * Should be used by `INode` only!
         * 
         * @param topic Topic
         * @param payload Payload (data)
         * @return true Delivery successful
         * @return false Delivery failed
         */
        virtual bool publish(std::string topic, std::string payload) = 0;
    };

    /**
     * @brief Generic node of SPSP
     * 
     * Implements common functionality for client and bridge node types.
     */
    class INode
    {
    protected:
        ILocalLayer* ll;

    public:
        /**
         * @brief Constructs a new Node object
         * 
         * @param ll Local layer
         */
        INode(ILocalLayer* ll) : ll{ll}
        {
            ll->setNode(this);
        };

        /**
         * @brief Receives the message from local layer
         * 
         * Acts as a callback for local layer receiver.
         * 
         * @param msg Received message
         */
        virtual void receiveLocal(Message msg) = 0;
    };
} // namespace SPSP
