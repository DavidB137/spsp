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
    const uint8_t BRIDGE_SUB_LIFETIME = 30;  // in minutes

    /**
     * @brief Bridge subscribe entry
     * 
     * Single entry in subscribe database of a bridge.
     */
    struct BridgeSubEntry
    {
        bool localLayer = true;
        uint8_t lifetime = BRIDGE_SUB_LIFETIME;
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

    protected:
        /**
         * @brief Processes PONG message
         * 
         * Doesn't do anything.
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processPong(const LocalMessage req) { return true; }

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
         * @brief Inserts entry into subscribe database
         * 
         * @param topic Topic
         * @param src Source node
         * @param localLayer True if this is local layer subscribe, false if this node's.
         * @param lifetime Lifetime of subscribe in minutes
         * @return true New topic - noone else is subscribed
         * @return false Subscribe to topic already exists
         */
        bool subDBInsert(const std::string topic, const LocalAddr src,
                         bool localLayer = true,
                         uint8_t lifetime = BRIDGE_SUB_LIFETIME);

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
