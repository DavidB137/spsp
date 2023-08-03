/**
 * @file spsp_client_sub_db.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Client subscribe database of SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <mutex>
#include <unordered_map>

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
} // namespace SPSP::Nodes
