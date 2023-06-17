/**
 * @file spsp.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Simple publish-subscribe protocol
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef SPSP_HPP
#define SPSP_HPP

#include <cassert>

#include "spsp_message.hpp"

namespace SPSP
{
    // Forward declaration
    class Node;

    /**
     * @brief Generic interface for local or far layer
     * 
     * Has a pointer to owner node (to directly call `Node::receive()` method). 
     */
    class LocalOrFarLayer
    {
    private:
        Node* node;

    public:
        /**
         * @brief Constructs a new layer object
         * 
         */
        LocalOrFarLayer() : node{nullptr} {};

        /**
         * @brief Sets the pointer to the owner node.
         * 
         * @param n Owner node
         */
        void setNode(Node* n) { node = n; };

        /**
         * @brief Gets the node object
         * 
         * Asserts the node is not NULL.
         * 
         * @return Node* Node pointer
         */
        Node* getNode()
        {
            assert(node != nullptr);
            return node;
        }
    };

    /**
     * @brief Interface for local layer
     * 
     */
    class LocalLayer : public LocalOrFarLayer
    {
    };

    /**
     * @brief Interface for far layer
     * 
     */
    class FarLayer : public LocalOrFarLayer
    {
    };

    /**
     * @brief Generic node of SPSP
     * 
     * Implements common functionality for client and bridge node types.
     */
    class Node
    {
        friend class LocalLayer;

    protected:
        LocalLayer ll;
        FarLayer fl;

    public:
        /**
         * @brief Constructs a new Node object
         * 
         * @param ll Local layer
         * @param fl Far layer
         */
        Node(LocalLayer ll, FarLayer fl) : ll{ll}, fl{fl} { ll.setNode(this); };

        /**
         * @brief Initializes the node
         * 
         */
        virtual void init() = 0;
    
    protected:
        /**
         * @brief Receives the message from local layer
         * 
         * Acts as a callback for local layer receiver.
         * 
         * @param msg Received message
         * @return Message Reply that should be sent back
         */
        virtual Message receive(Message msg) = 0;
    };
} // namespace SPSP

#endif
