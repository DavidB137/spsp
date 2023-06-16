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
    protected:
        LocalLayer ll;
        FarLayer fl;
    public:
        Node(LocalLayer ll, FarLayer fl) : ll{ll}, fl{fl} { ll.setNode(this); };
    };

    /**
     * @brief Client node
     * 
     */
    class Client : public Node
    {
        using Node::Node;
    };

    /**
     * @brief Bridge node
     * 
     */
    class Bridge : public Node
    {
        using Node::Node;
    };

    /**
     * @brief Client and bridge node
     * 
     */
    class ClientBridge : public Client, public Bridge
    {
        using Bridge::Bridge;
    };
} // namespace SPSP

#endif
