/**
 * @file spsp_node.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Node interface for SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <climits>

#include "spsp_layers.hpp"
#include "spsp_local_message.hpp"

namespace SPSP
{
    /**
     * @brief Subscribe callback type
     * 
     */
    using SubscribeCb = void (*)(const std::string topic, const std::string payload);

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
         * The pointer must be valid until the node is destroyed
         * or `unsetLocalLayer` is called.
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
         * @param rssi Received signal strength indicator (in dBm)
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool receiveLocal(const LocalMessage msg, int rssi = INT_MAX);

        /**
         * @brief Publishes payload to topic
         * 
         * This is primary endpoint for publishing data locally on all node types.
         * 
         * @param topic Topic
         * @param payload Payload
         * @return true Delivery successful
         * @return false Delivery failed
         */
        virtual bool publish(const std::string topic, const std::string payload) = 0;

        /**
         * @brief Subscribes to topic
         * 
         * This is primary endpoint for subscribing locally on all node types.
         * 
         * @param topic Topic
         * @param cb Callback function
         */
        virtual bool subscribe(const std::string topic, SubscribeCb cb) = 0;

    protected:
        /**
         * @brief Sends the message to local layer
         * 
         * @param msg Message to send
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool sendLocal(const LocalMessage msg);

        /**
         * @brief Processes PROBE_REQ message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        virtual bool processProbeReq(const LocalMessage req) = 0;

        /**
         * @brief Processes PROBE_RES message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        virtual bool processProbeRes(const LocalMessage req) = 0;

        /**
         * @brief Processes PUB message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        virtual bool processPub(const LocalMessage req) = 0;

        /**
         * @brief Processes SUB_REQ message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        virtual bool processSubReq(const LocalMessage req) = 0;

        /**
         * @brief Processes SUB_DATA message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        virtual bool processSubData(const LocalMessage req) = 0;
    };
} // namespace SPSP
