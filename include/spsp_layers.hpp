/**
 * @file spsp_layers.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Local and far layers for SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include "spsp_message.hpp"

namespace SPSP
{
    // Forward declaration
    class INode;

    /**
     * @brief Generic interface for local or far layer
     * 
     * Has a pointer to owner node (to directly call `INode::receive*()` method).
     * When node is not set, it does nothing.
     */
    class ILocalOrFarLayer
    {
    private:
        INode* m_node = nullptr;

    public:
        /**
         * @brief Sets pointer to the owner node.
         * 
         * @param n Owner node
         */
        void setNode(INode* n) { m_node = n; }

        /**
         * @brief Unsets pointer to the owner node.
         * 
         */
        void unsetNode() { m_node = nullptr; }

        /**
         * @brief Gets the node object
         * 
         * @return Node pointer
         */
        inline INode* getNode() const { return m_node; }

        /**
         * @brief Checks whether the owner node is connected
         * 
         * @return true Node is connected
         * @return false Node is disconnected
         */
        inline bool nodeConnected() const { return m_node != nullptr; }
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
        virtual bool send(const Message msg) = 0;
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
        virtual bool publish(const std::string topic, const std::string payload) = 0;

        /**
         * @brief Subscribes to given topic
         * 
         * Should be used by `INode` only!
         * 
         * @param topic Topic
         * @return true Subscribe successful
         * @return false Subscribe failed
         */
        virtual bool subscribe(const std::string topic) = 0;

         /**
         * @brief Unsubscribes from given topic
         * 
         * Should be used by `INode` only!
         * 
         * @param topic Topic
         * @return true Unsubscribe successful
         * @return false Unsubscribe failed
         */
        virtual bool unsubscribe(const std::string topic) = 0;
    };
} // namespace SPSP
