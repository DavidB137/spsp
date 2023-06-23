/**
 * @file spsp_bridge.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Bridge node type of SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <unordered_map>

#include "spsp_layers.hpp"
#include "spsp_local_addr.hpp"
#include "spsp_node.hpp"

namespace SPSP::Nodes
{
    static const uint8_t BRIDGE_SUB_LIFETIME  = 30;         //!< Default subscribe lifetime
    static const uint8_t BRIDGE_SUB_NO_EXPIRE = UINT8_MAX;  //!< Subscribe lifetime for no expire

    /**
     * @brief Bridge subscribe entry
     * 
     * Single entry in subscribe database of a bridge.
     * 
     * Empty addresses are treated as local to this node.
     */
    struct BridgeSubEntry
    {
        uint8_t lifetime = BRIDGE_SUB_LIFETIME;  //!< Lifetime in minutes
        SPSP::SubscribeCb cb = nullptr;          //!< Callback for incoming data
    };

    using BridgeSubDB = std::unordered_map<std::string, std::unordered_map<LocalAddr, BridgeSubEntry>>;

    /**
     * @brief Bridge node
     * 
     */
    class Bridge : public SPSP::INode
    {
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
         */
        bool subscribe(const std::string topic, SubscribeCb cb);

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
         * @brief Inserts entry into subscribe database and subscribes to given
         *        topic (if needed).
         * 
         * @param topic Topic
         * @param src Source node
         * @param lifetime Lifetime of subscribe in minutes
         * @param cb Subscribe callback (only for local subscribes)
         * @return true Insert successful
         * @return false Insert failed
         */
        bool subDBInsert(const std::string topic, const LocalAddr src,
                         uint8_t lifetime = BRIDGE_SUB_LIFETIME,
                         SubscribeCb cb = nullptr);

        /**
         * @brief Removes entry from subscribe database and unsubscribes from
         *        given topic (if needed).
         * 
         * @param topic Topic
         * @param src Source node
         * @return true Removal successful
         * @return false Removal failed
         */
        bool subDBRemove(const std::string topic, const LocalAddr src);

        /**
         * @brief Time tick callback for subscribe DB
         * 
         * Decrements subscribe database lifetimes.
         * If any item completely expires, removes it from DB and if it was
         * the last one for given topic, unsubscribes from it.
         */
        void subDBTick();
    };
} // namespace SPSP::Nodes
