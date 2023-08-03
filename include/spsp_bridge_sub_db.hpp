/**
 * @file spsp_bridge_sub_db.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Bridge subscribe database of SPSP
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
    static const uint8_t BRIDGE_SUB_LIFETIME  = 15;         //!< Default subscribe lifetime
    static const uint8_t BRIDGE_SUB_NO_EXPIRE = UINT8_MAX;  //!< Subscribe lifetime for no expire

    // Forward declaration
    class Bridge;

    /**
     * @brief Container for subscribe database of bridge
     * 
     */
    class BridgeSubDB
    {
        /**
         * @brief Bridge subscribe entry
         * 
         * Single entry in subscribe database of a bridge.
         * 
         * Empty addresses are treated as local to this node.
         */
        struct Entry
        {
            uint8_t lifetime = BRIDGE_SUB_LIFETIME;  //!< Lifetime in minutes
            SPSP::SubscribeCb cb = nullptr;          //!< Callback for incoming data
        };

        Bridge* m_bridge;                         //!< Pointer to bridge (owner)
        std::mutex m_mutex;                       //!< Mutex to prevent race conditions
        void* m_timer;                            //!< Timer handle pointer (platform dependent)
        std::unordered_map<
            std::string,
            std::unordered_map<LocalAddr, Entry>
        > m_db;                                   //!< Database
    
    public:
        /**
         * @brief Construct a new bridge sub DB
         * 
         * @param bridge Pointer to bridge node (owner)
         */
        BridgeSubDB(Bridge* bridge);

        /**
         * @brief Destroys the bridge sub DB
         */
        ~BridgeSubDB();

        /**
         * @brief Inserts entry into database
         * 
         * Internally subscribes on far layer if needed.
         * 
         * @param topic Topic
         * @param addr Node address
         * @param cb Callback for incoming data (only for local subscriptions)
         * @return true Insert (including subscribe) successful
         * @return false Insert failed
         */
        bool insert(const std::string topic, const LocalAddr addr,
                    const SubscribeCb cb = nullptr);

        /**
         * @brief Removes entry from database
         * 
         * Calls `removeUnusedTopics()`.
         * 
         * @param topic Topic
         * @param addr Node address
         */
        void remove(const std::string topic, const LocalAddr addr);

        /**
         * @brief Resubscribes to all topics
         * 
         */
        void resubscribeAll();

        /**
         * @brief Calls callbacks for incoming data
         * 
         * For non-local addresses calls `Bridge::publishSubData()`.
         * 
         * @param topic Topic
         * @param payload Data
         */
        void callCbs(const std::string topic, const std::string payload);

        /**
         * @brief Time tick callback
         * 
         * Decrements subscribe database lifetimes.
         * If any entry expires, removes it and if it was the last one for
         * given topic, unsubscribes from it.
         * In case of unsubscribe calls `Bridge::unsubscribeFar()`.
         */
        void tick();
    
    protected:
        /**
         * @brief Decrements lifetimes of entries
         * 
         */
        void decrementLifetimes();

        /**
         * @brief Removes expired entries
         * 
         */
        void removeExpiredSubEntries();

        /**
         * @brief Removes and unsubscribes from unused topics
         * 
         * In case of unsubscribe calls `Bridge::unsubscribeFar()`.
         */
        void removeUnusedTopics();
    };
} // namespace SPSP::Nodes
