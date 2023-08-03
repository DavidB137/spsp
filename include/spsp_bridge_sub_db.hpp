/**
 * @file spsp_bridge_sub_db.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Bridge subscribe database of SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <chrono>
#include <mutex>
#include <unordered_map>

#include "spsp_logger.hpp"
#include "spsp_node.hpp"
#include "spsp_timer.hpp"

// Log tag
#define SPSP_LOG_TAG "SPSP/Bridge/SubDB"

namespace SPSP::Nodes
{
    static const uint8_t BRIDGE_SUB_LIFETIME  = 15;         //!< Default subscribe lifetime
    static const uint8_t BRIDGE_SUB_NO_EXPIRE = UINT8_MAX;  //!< Subscribe lifetime for no expire

    // Forward declaration
    template <typename TLocalLayer, typename TFarLayer> class Bridge;

    /**
     * @brief Container for subscribe database of bridge
     * 
     */
    template <typename TLocalLayer, typename TFarLayer>
    class BridgeSubDB
    {
        using BridgeT = typename SPSP::Bridge<TLocalLayer, TFarLayer>;

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

        BridgeT* m_bridge;                        //!< Pointer to bridge (owner)
        std::mutex m_mutex;                       //!< Mutex to prevent race conditions
        Timer m_timer;                            //!< Timer handle pointer (platform dependent)
        std::unordered_map<
            std::string,
            std::unordered_map<BridgeT::LocalAddrT, Entry>
        > m_db;                                   //!< Database
    
    public:
        /**
         * @brief Construct a new bridge sub DB
         * 
         * @param bridge Pointer to bridge node (owner)
         */s
        BridgeSubDB(BridgeT* bridge)
            : m_bridge{bridge},
              m_timer{std::chrono::minutes(1),
                      std::bind(&BridgeSubDB<TLocalLayer, TFarLayer>::tick, this)},
              m_db{}
        {
            SPSP_LOGD("Initialized");
        }

        /**
         * @brief Destroys the bridge sub DB
         */
        ~BridgeSubDB()
        {
            SPSP_LOGD("Deinitialized");
        }

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
        bool insert(const std::string topic, const BridgeT::LocalAddrT addr,
                    SPSP::SubscribeCb cb)
        {
            const std::lock_guard lock(m_mutex);

            // Create sub entry
            Entry subEntry = {};
            if (addr.empty()) {
                subEntry.lifetime = BRIDGE_SUB_NO_EXPIRE;
                subEntry.cb = cb;
            }

            bool newTopic = m_db.find(topic) == m_db.end();

            if (newTopic) {
                // Subscribe to the topic
                if (!m_bridge->subscribeFar(topic)) {
                    SPSP_LOGE("Insert: subscribe to topic '%s' failed - not inserting anything",
                              topic.c_str());
                    return false;
                }
            }

            // Add/update the entry
            m_db[topic][addr] = subEntry;

            if (addr.empty()) {
                SPSP_LOGD("Inserted local entry for topic '%s' with%s callback (no expiration)",
                          topic.c_str(), (cb == nullptr ? "out" : ""));
            } else {
                SPSP_LOGD("Inserted '%s@%s' (expires in %d min)",
                          addr.str.c_str(), topic.c_str(), subEntry.lifetime);
            }

            return true;
        }

        /**
         * @brief Removes entry from database
         * 
         * Calls `removeUnusedTopics()`.
         * 
         * @param topic Topic
         * @param addr Node address
         */
        void remove(const std::string topic, const BridgeT::LocalAddrT addr)
        {
            {
                const std::scoped_lock lock(m_mutex);

                // Topic doesn't exist
                if (m_db.find(topic) == m_db.end()) return;

                m_db[topic].erase(addr);

                SPSP_LOGD("Removed addr %s on topic '%s'",
                          addr.empty() ? "." : addr.str.c_str(), topic.c_str());
            }

            // Unsubscribe if not needed anymore
            this->removeUnusedTopics();
        }

        /**
         * @brief Resubscribes to all topics
         * 
         */
        void resubscribeAll()
        {
            const std::lock_guard lock(m_mutex);

            for (auto const& [topic, topicEntry] : m_db) {
                if (!m_bridge->subscribeFar(topic)) {
                    SPSP_LOGW("Resubscribe to topic %s failed", topic.c_str());
                }
            }

            SPSP_LOGD("Resubscribed to %d topics", m_db.size());
        }

        /**
         * @brief Calls callbacks for incoming data
         * 
         * For non-local addresses calls `Bridge::publishSubData()`.
         * 
         * @param topic Topic
         * @param payload Data
         */
        void callCbs(const std::string topic, const std::string payload)
        {
            m_mutex.lock();

            if (auto topicIt = m_db.find(topic); topicIt != m_db.end()) {
                for (auto const& [addr, subEntry] : topicIt->second) {
                    m_mutex.unlock();

                    if (addr.empty()) {
                        // This node's subscription - call callback
                        SPSP_LOGD("Calling user callback for topic '%s'",
                                    topic.c_str());
                        subEntry.cb(topic, payload);
                    } else {
                        // Local layer subscription
                        m_bridge->publishSubData(addr, topic, payload);
                    }

                    m_mutex.lock();
                }
            }

            m_mutex.unlock();
        }

        /**
         * @brief Time tick callback
         * 
         * Decrements subscribe database lifetimes.
         * If any entry expires, removes it and if it was the last one for
         * given topic, unsubscribes from it.
         * In case of unsubscribe calls `Bridge::unsubscribeFar()`.
         */
        void tick()
        {
            SPSP_LOGD("Tick running");

            this->decrementLifetimes();
            this->removeExpiredSubEntries();
            this->removeUnusedTopics();

            SPSP_LOGD("Tick done");
        }
    
    protected:
        /**
         * @brief Decrements lifetimes of entries
         * 
         */
        void decrementLifetimes()
        {
            const std::lock_guard lock(m_mutex);

            for (auto const& [topic, topicEntry] : m_db) {
                for (auto const& [addr, subEntry] : topicEntry) {
                    // Don't decrement entries with infinite lifetime
                    if (subEntry.lifetime != BRIDGE_SUB_NO_EXPIRE) {
                        m_db[topic][addr].lifetime--;
                    }
                }
            }
        }

        /**
         * @brief Removes expired entries
         * 
         */
        void removeExpiredSubEntries()
        {
            const std::lock_guard lock(m_mutex);

            for (auto const& [topic, topicEntry] : m_db) {
                auto subEntryIt = topicEntry.begin();
                while (subEntryIt != topicEntry.end()) {
                    auto src = subEntryIt->first;

                    if (subEntryIt->second.lifetime == 0) {
                        // Expired
                        subEntryIt = m_db[topic].erase(subEntryIt);
                        SPSP_LOGD("Removed src %s from topic '%s'",
                                  src.str.c_str(), topic.c_str());
                    } else {
                        // Continue
                        subEntryIt++;
                    }
                }
            }
        }

        /**
         * @brief Removes and unsubscribes from unused topics
         * 
         * In case of unsubscribe calls `Bridge::unsubscribeFar()`.
         */
        void removeUnusedTopics()
        {
            const std::lock_guard lock(m_mutex);

            auto topicEntryIt = m_db.begin();
            while (topicEntryIt != m_db.end()) {
                auto topic = topicEntryIt->first;

                // If unused
                if (topicEntryIt->second.empty()) {
                    bool unsubSuccess = m_bridge->unsubscribeFar(topic);

                    if (unsubSuccess) {
                        topicEntryIt = m_db.erase(topicEntryIt);
                        SPSP_LOGD("Removed unused topic '%s'", topic.c_str());
                    } else {
                        SPSP_LOGE("Topic '%s' can't be unsubscribed. Will try again in next tick.",
                                topic.c_str());
                    }
                } else {
                    topicEntryIt++;
                }
            }
        }
    };
} // namespace SPSP::Nodes

#undef SPSP_LOG_TAG
