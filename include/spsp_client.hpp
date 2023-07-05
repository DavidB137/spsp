/**
 * @file spsp_client.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Client node type of SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <unordered_map>

#include "spsp_layers.hpp"
#include "spsp_node.hpp"

namespace SPSP::Nodes
{
    static const uint8_t CLIENT_SUB_LIFETIME = 10;  //!< Default subscribe lifetime

    /**
     * @brief Client subscribe entry
     * 
     * Single entry in subscribe database of a client.
     */
    struct ClientSubEntry
    {
        uint8_t lifetime = CLIENT_SUB_LIFETIME;  //!< Lifetime in minutes
        SPSP::SubscribeCb cb = nullptr;          //!< Callback for incoming data
    };

    using ClientSubDB = std::unordered_map<std::string, ClientSubEntry>;

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
         * Subscribe request is sent to the bridge and extended automatically
         * until `unsubscribe()` is called.
         * 
         * @param topic Topic
         * @param cb Callback function
         * @return true Subscribe successful
         * @return false Subscribe failed
         */
        bool subscribe(const std::string topic, SubscribeCb cb);

        /**
         * @brief Unsubscribes from topic
         * 
         * This is primary endpoint for unsubscribing locally on this node.
         * 
         * @param topic Topic
         * @return true Unsubscribe successful
         * @return false Unsubscribe failed
         */
        bool unsubscribe(const std::string topic);

        /**
         * @brief Predicate whether this node is a bridge
         * 
         * @return true This is a bridge
         * @return false This is not a bridge
         */
        inline bool isBridge() { return false; }

        /**
         * @brief Time tick callback for subscribe DB
         * 
         * Decrements subscribe database lifetimes.
         * If any item completely expires, removes it from DB.
         */
        void subDBTick();

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
         * Doesn't do anything.
         * This is handled internally by concrete local layer.
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processProbeRes(const LocalMessage req) { return true; }

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
