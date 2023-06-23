/**
 * @file spsp_client.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Client node type of SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include "spsp_layers.hpp"
#include "spsp_node.hpp"

#include <unordered_map>

namespace SPSP::Nodes
{
    using ClientSubDB = std::unordered_map<LocalAddr, SubscribeCb>;

    /**
     * @brief Client node
     * 
     */
    class Client : public SPSP::INode
    {
        ClientSubDB m_subDB;

    public:
        /**
         * @brief Constructs a new client node
         * 
         */
        Client();

        /**
         * @brief Destroys the client node
         * 
         */
        ~Client();

        /**
         * @brief Publishes payload to topic
         * 
         * This is primary endpoint for publishing locally data on this node.
         * Sends data to the local layer.
         * 
         * @param topic Topic
         * @param payload Payload
         * @return true Delivery successful
         * @return false Delivery failed
         */
        bool publish(const std::string topic, const std::string payload);

        /**
         * @brief Subscribes to topic
         * 
         * This is primary endpoint for subscribing locally on this node.
         * Directly forwards incoming data from local layer to given callback.
         * 
         * @param topic Topic
         * @param cb Callback function
         */
        bool subscribe(const std::string topic, SubscribeCb cb);

        /**
         * @brief Predicate whether this node is a bridge
         * 
         * @return true This is a bridge
         * @return false This is not a bridge
         */
        inline bool isBridge() { return false; }
    
    protected:
        /**
         * @brief Processes PROBE_REQ message
         * 
         * Doesn't do anything.
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processProbeReq(const LocalMessage req) { return true; }

        /**
         * @brief Processes PROBE_RES message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processProbeRes(const LocalMessage req);

        /**
         * @brief Processes PUB message
         * 
         * Doesn't do anything.
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processPub(const LocalMessage req) { return true; }

        /**
         * @brief Processes SUB_REQ message
         * 
         * Doesn't do anything.
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processSubReq(const LocalMessage req) { return true; }

        /**
         * @brief Processes SUB_DATA message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processSubData(const LocalMessage req);
    };
} // namespace SPSP::Nodes
