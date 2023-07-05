/**
 * @file spsp_bridge.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Bridge node type of SPSP
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
         * Doesn't subscribe on far layer (this is caller's responsibility).
         * 
         * @param topic Topic
         * @param addr Node address
         * @param cb Callback for incoming data (only for local subscriptions)
         * @return true New topic
         * @return false Someone else has already been subscribed to this topic
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

    /**
     * @brief Bridge node
     * 
     */
    class Bridge : public SPSP::INode
    {
        // Subscribe DB can access private members
        friend class BridgeSubDB;

    protected:
        SPSP::IFarLayer* m_fl = nullptr;
        BridgeSubDB m_subDB;

    public:
        /**
         * @brief Constructs a new bridge node
         * 
         */
        Bridge();

        /**
         * @brief Destroys the bridge node
         * 
         */
        ~Bridge();

        /**
         * @brief Sets pointer to the far layer.
         * 
         * The pointer must be valid until the `Bridge` is destroyed
         * or `unsetFarLayer` is called.
         * Safe to call even when far layer is already set.
         * 
         * @param fl New far layer
         */
        void setFarLayer(IFarLayer* fl);

        /**
         * @brief Unsets pointer to the far layer.
         * 
         */
        void unsetFarLayer();

        /**
         * @brief Checks whether the far layer is connected
         * 
         * @return true Far layer is connected
         * @return false Far layer is disconnected
         */
        inline bool farLayerConnected() const { return m_fl != nullptr; }

        /**
         * @brief Receives the message from far layer
         * 
         * Acts as a callback for far layer receiver.
         * 
         * @param topic Topic
         * @param payload Payload (data)
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool receiveFar(const std::string topic, const std::string payload);

        /**
         * @brief Publishes payload to topic
         * 
         * This is primary endpoint for publishing locally data on this node.
         * Directly sends data to far layer.
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
         * Directly forwards incoming data from far layer to given callback.
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
        inline bool isBridge() { return true; }

    protected:
        /**
         * @brief Processes PROBE_REQ message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processProbeReq(const LocalMessage req);

        /**
         * @brief Processes PROBE_RES message
         * 
         * Doesn't do anything.
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processProbeRes(const LocalMessage req) { return true; }

        /**
         * @brief Processes PUB message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processPub(const LocalMessage req);

        /**
         * @brief Processes SUB_REQ message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processSubReq(const LocalMessage req);

        /**
         * @brief Processes SUB_DATA message
         * 
         * Doesn't do anything.
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processSubData(const LocalMessage req) { return true; }

        /**
         * @brief Publishes received subscription data to local layer node
         * 
         * @param addr Node address
         * @param topic Topic
         * @param payload Payload
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool publishSubData(const LocalAddr addr, const std::string topic,
                            const std::string payload);

        /**
         * @brief Subscribes to topic on far layer (if connected)
         * 
         * @param topic Topic
         * @return true Subscription successful
         * @return false Subscription failed
         */
        bool subscribeFar(const std::string topic);

        /**
         * @brief Unsubscribes from topic on far layer (if connected)
         * 
         * @param topic Topic
         * @return true Unsubscription successful
         * @return false Unsubscription failed
         */
        bool unsubscribeFar(const std::string topic);
    };
} // namespace SPSP::Nodes
