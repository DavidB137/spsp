/**
 * @file spsp_node.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Node interface for SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include "spsp_layers.hpp"
#include "spsp_message.hpp"

namespace SPSP
{
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
        void setLocalLayer(ILocalLayer* ll);

        /**
         * @brief Unsets pointer to the local layer.
         * 
         */
        void unsetLocalLayer();

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
        bool receiveLocal(const Message msg);

    protected:
        /**
         * @brief Sends the message to local layer
         * 
         * @param msg Message to send
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool sendLocal(const Message msg);

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
        bool processPing(const Message req);

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
