/**
 * @file local_broker.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Local broker far layer for SPSP
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <mutex>
#include <string>

#include "spsp/layers.hpp"
#include "spsp/node.hpp"
#include "spsp/wildcard_trie.hpp"

namespace SPSP::FarLayers::LocalBroker
{
    /**
     * @brief Local broker far layer
     *
     * Acts as local MQTT server.
     */
    class LocalBroker : public IFarLayer
    {
        std::mutex m_mutex;
        SPSP::WildcardTrie<bool> m_subs;  //!< Subscriptions
        std::string m_topicPrefix;        //!< Topic prefix for publishing

    public:
        /**
         * @brief Constructs a new local broker object
         *
         * @param topicPrefix Topic prefix for publishing
         */
        LocalBroker(const std::string topicPrefix = "spsp");

        /**
         * @brief Destroys local broker layer object
         *
         */
        ~LocalBroker();

        /**
         * @brief Publishes message coming from node
         *
         * @param src Source address
         * @param topic Topic
         * @param payload Payload (data)
         * @return true Delivery successful
         * @return false Delivery failed
         */
        bool publish(const std::string& src, const std::string& topic,
                     const std::string& payload);

        /**
         * @brief Subscribes to given topic
         *
         * Should be used by `INode` only!
         *
         * @param topic Topic
         * @return true Subscribe successful
         * @return false Subscribe failed
         */
        bool subscribe(const std::string& topic);

        /**
         * @brief Unsubscribes from given topic
         *
         * Should be used by `INode` only!
         *
         * @param topic Topic
         * @return true Unsubscribe successful
         * @return false Unsubscribe failed
         */
        bool unsubscribe(const std::string& topic);
    };
} // namespace SPSP::FarLayers::LocalBroker
