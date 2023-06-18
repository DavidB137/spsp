/**
 * @file spsp.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Simple publish-subscribe protocol
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
    };

    /**
     * @brief Generic node of SPSP
     * 
     * Implements common functionality for client and bridge node types.
     * Composed of 1 local layer (ESP-NOW) and 1 far layer (MQTT).
     * If you need multiple local layer, just create multiple instances of the node.
     */
    class INode
    {
    protected:
        ILocalLayer* m_ll = nullptr;

    public:
        /**
         * @brief Sets pointer to the local layer.
         * 
         * Safe to call even when local layer is already set.
         * 
         * @param ll New local layer
         */
        void setLocalLayer(ILocalLayer* ll)
        {
            // Unset old local layer
            if (m_ll != nullptr) this->unsetLocalLayer();

            m_ll = ll;
            m_ll->setNode(this);
        }

        /**
         * @brief Unsets pointer to the local layer.
         * 
         */
        void unsetLocalLayer()
        {
            if (m_ll != nullptr) {
                m_ll->unsetNode();
                m_ll = nullptr;
            }
        }

        /**
         * @brief Checks whether the local layer is connected
         * 
         * @return true Local layer is connected
         * @return false Local layer is disconnected
         */
        inline bool localLayerConnected() const { return m_ll != nullptr; }

        /**
         * @brief Receives the message from local layer
         * 
         * Acts as a callback for local layer receiver.
         * 
         * @param msg Received message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        virtual bool receiveLocal(const Message msg) = 0;

    protected:
        /**
         * @brief Sends the message to local layer
         * 
         * @param msg Message to send
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool sendLocal(const Message msg)
        {
            // Local layer is not connected - can't deliver
            if (!this->localLayerConnected()) return false;

            // Send to local layer
            return m_ll->send(msg);
        }

        /**
         * @brief Processes PING message
         * 
         * Sends back PONG message.
         * 
         * This is universal for all nodes, so method's implementation is
         * present.
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processPing(const Message req)
        {
            Message res = req;
            res.type = MessageType::PONG;
            return this->sendLocal(res);
        }

        /**
         * @brief Processes PONG message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        virtual bool processPong(const Message req) = 0;

        /**
         * @brief Processes PUB message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        virtual bool processPub(const Message req) = 0;

        /**
         * @brief Processes SUB_REQ message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        virtual bool processSubReq(const Message req) = 0;

        /**
         * @brief Processes SUB_DATA message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        virtual bool processSubData(const Message req) = 0;
    };
} // namespace SPSP
