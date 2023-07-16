/**
 * @file spsp_client.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Client node type of SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <mutex>
#include <thread>
#include <unordered_map>

#include "spsp_layers.hpp"
#include "spsp_local_addr.hpp"
#include "spsp_node.hpp"

namespace SPSP::Nodes
{
    static const uint8_t CLIENT_SUB_LIFETIME = 10;  //!< Default subscribe lifetime

    // Forward declaration
    class Client;

    /**
     * @brief Container for subscribe database of client
     * 
     */
    class ClientSubDB
    {
        /**
         * @brief Client subscribe entry
         * 
         * Single entry in subscribe database of a client.
         */
        struct Entry
        {
            uint8_t lifetime = CLIENT_SUB_LIFETIME;  //!< Lifetime in minutes
            SPSP::SubscribeCb cb = nullptr;          //!< Callback for incoming data
        };

        Client* m_client;                             //!< Pointer to client (owner)
        std::mutex m_mutex;                           //!< Mutex to prevent race conditions
        void* m_timer;                                //!< Timer handle pointer (platform dependent)
        std::unordered_map<std::string, Entry> m_db;  //!< Database

    public:
        /**
         * @brief Construct a new client sub DB
         * 
         * @param client Pointer to client node (owner)
         */
        ClientSubDB(Client* client);

        /**
         * @brief Destroys the client sub DB
         */
        ~ClientSubDB();

        /**
         * @brief Inserts entry into database
         * 
         * @param topic Topic
         * @param cb Callback for incoming data
         */
        void insert(const std::string topic, SPSP::SubscribeCb cb);

        /**
         * @brief Removes entry from database
         * 
         * @param topic Topic
         */
        void remove(const std::string topic);

        /**
         * @brief Calls callbacks for incoming data
         * 
         * @param topic Topic
         * @param payload Data
         */
        void callCb(const std::string topic, const std::string payload);

        /**
         * @brief Time tick callback
         * 
         * Decrements subscribe database lifetimes.
         * If any item expires, renews it.
         */
        void tick();
    };

    /**
     * @brief Client node
     * 
     */
    class Client : public SPSP::INode
    {
        // Subscribe DB can access private members
        friend class ClientSubDB;

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
         * @brief Predicate whether this node is a client
         * 
         * @return true This is a client
         * @return false This is not a client
         */
        inline bool isClient() { return true; }

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
        bool processProbeReq(const LocalMessage req) { return false; }

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
        bool processPub(const LocalMessage req) { return false; }

        /**
         * @brief Processes SUB_REQ message
         * 
         * Doesn't do anything.
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processSubReq(const LocalMessage req) { return false; }

        /**
         * @brief Processes SUB_DATA message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processSubData(const LocalMessage req);

        /**
         * @brief Processes UNSUB message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processUnsub(const LocalMessage req) { return false; }
    };
} // namespace SPSP::Nodes
