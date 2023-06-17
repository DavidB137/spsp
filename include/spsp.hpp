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

#include "spsp_message.hpp"

namespace SPSP
{
    // Forward declaration
    class Node;

    /**
     * @brief Interface for local layer
     * 
     */
    class LocalLayer
    {
    protected:
        Node* node;

    public:
        /**
         * @brief Constructs a new local layer object
         * 
         */
        LocalLayer() : node{nullptr} {};

        /**
         * @brief Sets the pointer to the owner node.
         * 
         * @param n 
         */
        void setNode(Node* n) { node = n; };
    };

    /**
     * @brief Interface for far layer
     * 
     */
    class FarLayer
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

    /**
     * @brief Client node
     * 
     */
    class Client : public Node
    {
    public:
        using Node::Node;

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
    class Bridge : public Node
    {
    public:
        using Node::Node;

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
} // namespace SPSP

#endif
