/**
 * @file spsp_client.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Client node type of SPSP
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
#include "spsp_wildcard_trie.hpp"

// Log tag
#define SPSP_LOG_TAG "SPSP/Client"

namespace SPSP::Nodes
{
    static const auto CLIENT_SUB_LIFETIME = std::chrono::minutes(10);  //!< Default subscribe lifetime

    /**
     * @brief Client node
     *
     * @tparam TLocalLayer Type of local layer
     */
    template <typename TLocalLayer>
    class Client : public ILocalNode<TLocalLayer>
    {
    protected:
        /**
         * @brief Client subscribe database entry
         *
         * Single entry in subscribe database of a client.
         */
        struct SubDBEntry
        {
            std::chrono::minutes lifetime = CLIENT_SUB_LIFETIME;  //!< Lifetime in minutes
            SPSP::SubscribeCb cb = nullptr;                       //!< Callback for incoming data
        };

        std::mutex m_mutex;                //!< Mutex to prevent race conditions
        WildcardTrie<SubDBEntry> m_subDB;  //!< Subscribe database
        Timer m_subDBTimer;                //!< Sub DB timer

    public:
        using LocalAddrT = TLocalLayer::LocalAddrT;
        using LocalMessageT = TLocalLayer::LocalMessageT;

        /**
         * @brief Constructs a new client node
         *
         * @param ll Local layer
         */
        Client(TLocalLayer* ll)
            : ILocalNode<TLocalLayer>{ll}, m_subDB{},
              m_subDBTimer{std::chrono::minutes(1),
                           std::bind(&Client<TLocalLayer>::subDBTick, this)}
        {
            SPSP_LOGI("Initialized");
        }

        /**
         * @brief Destroys the client node
         *
         */
        ~Client()
        {
            SPSP_LOGI("Deinitialized");
        }

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
        bool publish(const std::string& topic, const std::string& payload)
        {
            SPSP_LOGD("Publishing: topic '%s', payload '%s'",
                      topic.c_str(), payload.c_str());

            LocalMessageT msg = {};
            // msg.addr is empty => send to the bridge node
            msg.type = LocalMessageType::PUB;
            msg.topic = topic;
            msg.payload = payload;

            return this->sendLocal(msg);
        }

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
        bool subscribe(const std::string& topic, SubscribeCb cb)
        {
            SPSP_LOGD("Subscribing to topic '%s'", topic.c_str());

            if (this->sendSubscribe(topic)) {
                // Subscribe request delivered successfully
                const std::scoped_lock lock(m_mutex);

                // Add to sub DB
                SubDBEntry subDBEntry = { .cb = cb };
                m_subDB.insert(topic, subDBEntry);

                return true;
            }

            return false;
        }

        /**
         * @brief Unsubscribes from topic
         *
         * This is primary endpoint for unsubscribing locally on this node.
         *
         * @param topic Topic
         * @return true Unsubscribe successful
         * @return false Unsubscribe failed
         */
        bool unsubscribe(const std::string& topic)
        {
            SPSP_LOGD("Unsubscribing from topic '%s'", topic.c_str());

            LocalMessageT msg = {};
            // msg.addr is empty => send to the bridge node
            msg.type = LocalMessageType::UNSUB;
            msg.topic = topic;
            // msg.payload is empty

            // Remove from sub DB
            m_subDB.remove(topic);

            // Explicitly unsubscribe from bridge
            // If this fails, the timeout on bridge will just expire in
            // couple of minutes.
            this->sendLocal(msg);

            return true;
        }

    protected:
        /**
         * @brief Processes PROBE_REQ message
         *
         * Doesn't do anything.
         *
         * @param req Request message
         * @param rssi Received signal strength indicator (in dBm)
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processProbeReq(const LocalMessageT& req,
                             int rssi = NODE_RSSI_UNKNOWN) { return false; }

        /**
         * @brief Processes PROBE_RES message
         *
         * Just publishes RSSI.
         * This is handled internally by concrete local layer.
         *
         * @param req Request message
         * @param rssi Received signal strength indicator (in dBm)
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processProbeRes(const LocalMessageT& req,
                             int rssi = NODE_RSSI_UNKNOWN)
        {
            this->publishRssi(req.addr, rssi);
            return true;
        }

        /**
         * @brief Processes PUB message
         *
         * Doesn't do anything.
         *
         * @param req Request message
         * @param rssi Received signal strength indicator (in dBm)
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processPub(const LocalMessageT& req,
                        int rssi = NODE_RSSI_UNKNOWN) { return false; }

        /**
         * @brief Processes SUB_REQ message
         *
         * Doesn't do anything.
         *
         * @param req Request message
         * @param rssi Received signal strength indicator (in dBm)
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processSubReq(const LocalMessageT& req,
                           int rssi = NODE_RSSI_UNKNOWN) { return false; }

        /**
         * @brief Processes SUB_DATA message
         *
         * @param req Request message
         * @param rssi Received signal strength indicator (in dBm)
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processSubData(const LocalMessageT& req,
                            int rssi = NODE_RSSI_UNKNOWN)
        {
            // Get matching entries
            std::unordered_map<std::string, const SubDBEntry&> entries;
            {
                const std::scoped_lock lock(m_mutex);
                entries = m_subDB.find(req.topic);
            }

            for (auto& [topic, entry] : entries) {
                SPSP_LOGD("Calling user callback for topic '%s'",
                          topic.c_str());
                cb(topic, req.payload);
            }

            return true;
        }

        /**
         * @brief Processes UNSUB message
         *
         * @param req Request message
         * @param rssi Received signal strength indicator (in dBm)
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processUnsub(const LocalMessageT& req,
                          int rssi = NODE_RSSI_UNKNOWN) { return false; }

        /**
         * @brief Prepares and sends SUB_REQ message to local layer
         *
         * @param topic Topic
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool sendSubscribe(const std::string& topic)
        {
            LocalMessageT msg = {};
            // msg.addr is empty => send to the bridge node
            msg.type = LocalMessageType::SUB_REQ;
            msg.topic = topic;
            // msg.payload is empty

            return this->sendLocal(msg);
        }

        /**
         * @brief Subscribe DB timer tick callback
         *
         * Decrements subscribe database lifetimes.
         * If any item expires, renews it.
         */
        void subDBTick()
        {
            const std::scoped_lock lock(m_mutex);

            SPSP_LOGD("SubDB: Tick running");

            m_subDB.forEach([this](const std::string& topic, SubDBEntry& entry) {
                entry.lifetime--;

                if (entry.lifetime == 0) {
                    SPSP_LOGD("SubDB: Topic '%s' expired (renewing)", topic.c_str());

                    bool extended = this->sendSubscribe(topic);
                    if (!extended) {
                        SPSP_LOGE("SubDB: Topic '%s' can't be extended. Will try again in next tick.",
                                  topic.c_str());

                        entry.lifetime++;
                    }
                }
            });

            SPSP_LOGD("SubDB: Tick done");
        }
    };
} // namespace SPSP::Nodes

#undef SPSP_LOG_TAG
