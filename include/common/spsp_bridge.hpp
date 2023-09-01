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
#include <unordered_map>
#include <vector>

#include "spsp_local_addr_mac.hpp"
#include "spsp_logger.hpp"
#include "spsp_node.hpp"
#include "spsp_timer.hpp"
#include "spsp_wildcard_trie.hpp"

// Log tag
#define SPSP_LOG_TAG "SPSP/Bridge"

namespace SPSP::Nodes
{
    static const uint8_t BRIDGE_SUB_LIFETIME  = 15;         //!< Default subscribe lifetime
    static const uint8_t BRIDGE_SUB_NO_EXPIRE = UINT8_MAX;  //!< Subscribe lifetime for no expire

    /**
     * @brief Bridge configuration
     *
     * Everything here is optional.
     */
    struct BridgeConfig
    {
        struct Reporting
        {
            bool version = true;       //!< Report SPSP version during construction
            bool probePayload = true;  //!< Report payload of PROBE_REQ (on ESP-NOW contains SPSP version)
            bool rssiOnProbe = true;   //!< Report RSSI on PROBE_REQ
            bool rssiOnPub = true;     //!< Report RSSI on PUB
            bool rssiOnSub = true;     //!< Report RSSI on SUB_REQ
            bool rssiOnUnsub = true;   //!< Report RSSI on UNSUB
        };

        Reporting reporting;
    };

    /**
     * @brief Bridge node
     *
     * @tparam TLocalLayer Type of local layer
     * @tparam TFarLayer   Type of far layer
     */
    template <typename TLocalLayer, typename TFarLayer>
    class Bridge : public ILocalAndFarNode<TLocalLayer, TFarLayer>
    {
    public:
        using LocalAddrT = TLocalLayer::LocalAddrT;
        using LocalMessageT = TLocalLayer::LocalMessageT;

    protected:
        /**
         * @brief Bridge subscribe entry
         *
         * Single entry in subscribe database of a bridge.
         *
         * Empty addresses are treated as local to this node.
         */
        struct SubDBEntry
        {
            uint8_t lifetime = BRIDGE_SUB_LIFETIME;  //!< Lifetime in minutes
            SPSP::SubscribeCb cb = nullptr;          //!< Callback for incoming data
        };

        using SubDBMapT = std::unordered_map<LocalAddrT, SubDBEntry>;

        std::mutex m_mutex;               //!< Mutex to prevent race conditions
        BridgeConfig m_conf;              //!< Configuration
        WildcardTrie<SubDBMapT> m_subDB;  //!< Subscribe database
        Timer m_subDBTimer;               //!< Sub DB timer

    public:
        /**
         * @brief Construct a new bridge object
         *
         * @param ll Local layer
         * @param fl Far layer
         * @param conf Configuration
         */
        Bridge(TLocalLayer* ll, TFarLayer* fl, BridgeConfig conf = {})
            : ILocalAndFarNode<TLocalLayer, TFarLayer>{ll, fl},
              m_conf{conf},
              m_subDBTimer{std::chrono::minutes(1),
                           std::bind(&Bridge<TLocalLayer, TFarLayer>::subDBTick,
                           this)}
        {
            // Publish version
            if (conf.reporting.version) {
                this->publishVersion();
            }

            SPSP_LOGI("Initialized");
        }

        /**
         * @brief Destroys the bridge node
         *
         */
        ~Bridge()
        {
            SPSP_LOGI("Deinitialized");
        }

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
        bool receiveFar(const std::string& topic, const std::string& payload)
        {
            const std::scoped_lock lock(m_mutex);

            SPSP_LOGD("Received far msg: topic '%s', payload '%s'",
                      topic.c_str(), payload.c_str());

            // Get matching entries
            auto entries = m_subDB.find(topic);

            for (auto& [entryTopic, entryMap] : entries) {
                for (auto& [addr, entry] : entryMap) {
                    if (addr.empty()) {
                        // This node's subscription - call callback
                        SPSP_LOGD("Calling user callback for topic '%s' in new thread",
                                  topic.c_str());
                        std::thread t(entry.cb, topic, payload);
                        t.detach();
                    } else {
                        // Local layer subscription
                        std::thread t(&Bridge<TLocalLayer, TFarLayer>::publishSubData,
                                      this, addr, topic, payload);
                        t.detach();
                    }
                }
            }

            return true;
        }

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
        bool publish(const std::string& topic, const std::string& payload)
        {
            SPSP_LOGD("Publishing locally: topic '%s', payload '%s'",
                      topic.c_str(), payload.c_str());

            return this->getFarLayer()->publish(LocalAddrMAC::local().str,
                                                topic, payload);
        }

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
        bool subscribe(const std::string& topic, SubscribeCb cb)
        {
            const std::scoped_lock lock(m_mutex);

            SPSP_LOGD("Subscribing locally to topic '%s'", topic.c_str());

            m_subDB[topic][LocalAddrT{}] = SubDBEntry{.cb = cb};

            return true;
        }

        /**
         * @brief Resubscribes to all topics
         *
         */
        void resubscribeAll()
        {
            const std::scoped_lock lock(m_mutex);

            m_subDB.forEach(
                [this](const std::string& topic, const SubDBMapT& topicEntries) {
                    if (!this->getFarLayer()->subscribe(topic)) {
                        SPSP_LOGW("Resubscribe to topic %s failed",
                                  topic.c_str());
                    }
                }
            );
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
            SPSP_LOGD("Unsubscribing locally from topic '%s'", topic.c_str());

            {
                const std::scoped_lock lock(m_mutex);
                m_subDB[topic].erase(LocalAddrT{});
            }

            // Remove unused topics
            this->subDBRemoveUnusedTopics();

            return true;
        }

    protected:
        /**
         * @brief Processes PROBE_REQ message
         *
         * @param req Request message
         * @param rssi Received signal strength indicator (in dBm)
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processProbeReq(const LocalMessageT& req,
                             int rssi = NODE_RSSI_UNKNOWN)
        {
            LocalMessageT res = req;
            res.type = LocalMessageType::PROBE_RES;
            res.payload = SPSP::VERSION;

            // Publish RSSI
            if (m_conf.reporting.rssiOnProbe) {
                this->publishRssi(req.addr, rssi);
            }

            // Publish payload
            if (m_conf.reporting.probePayload) {
                std::string probePayloadReportTopic =
                    NODE_REPORTING_TOPIC + "/" +
                    NODE_REPORTING_PROBE_PAYLOAD_SUBTOPIC + "/" +
                    req.addr.str;

                this->publish(probePayloadReportTopic, req.payload);
            }

            return this->sendLocal(res);
        }

        /**
         * @brief Processes PROBE_RES message
         *
         * Doesn't do anything.
         *
         * @param req Request message
         * @param rssi Received signal strength indicator (in dBm)
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processProbeRes(const LocalMessageT& req,
                             int rssi = NODE_RSSI_UNKNOWN) { return false; }

        /**
         * @brief Processes PUB message
         *
         * @param req Request message
         * @param rssi Received signal strength indicator (in dBm)
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processPub(const LocalMessageT& req,
                        int rssi = NODE_RSSI_UNKNOWN)
        {
            // Publish RSSI
            if (m_conf.reporting.rssiOnPub) {
                this->publishRssi(req.addr, rssi);
            }

            return this->getFarLayer()->publish(req.addr.str, req.topic,
                                                req.payload);
        }

        /**
         * @brief Processes SUB_REQ message
         *
         * @param req Request message
         * @param rssi Received signal strength indicator (in dBm)
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processSubReq(const LocalMessageT& req,
                           int rssi = NODE_RSSI_UNKNOWN)
        {
            // Publish RSSI
            if (m_conf.reporting.rssiOnSub) {
                this->publishRssi(req.addr, rssi);
            }

            {
                const std::scoped_lock lock(m_mutex);
                m_subDB[req.topic][req.addr] = SubDBEntry{};
            }

            return true;
        }

        /**
         * @brief Processes SUB_DATA message
         *
         * Doesn't do anything.
         *
         * @param req Request message
         * @param rssi Received signal strength indicator (in dBm)
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processSubData(const LocalMessageT& req,
                            int rssi = NODE_RSSI_UNKNOWN) { return false; }

        /**
         * @brief Processes UNSUB message
         *
         * @param req Request message
         * @param rssi Received signal strength indicator (in dBm)
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processUnsub(const LocalMessageT& req,
                          int rssi = NODE_RSSI_UNKNOWN)
        {
            // Publish RSSI
            if (m_conf.reporting.rssiOnUnsub) {
                this->publishRssi(req.addr, rssi);
            }

            {
                const std::scoped_lock lock(m_mutex);
                m_subDB[req.topic].erase(req.addr);
            }

            // Remove unused topics
            this->subDBRemoveUnusedTopics();

            return true;
        }

        /**
         * @brief Publishes received subscription data to local layer node
         *
         * @param addr Node address
         * @param topic Topic
         * @param payload Payload
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool publishSubData(const LocalAddrT& addr, const std::string& topic,
                            const std::string& payload)
        {
            SPSP_LOGD("Sending SUB_DATA to %s: topic '%s', payload '%s'",
                      addr.str.c_str(), topic.c_str(), payload.c_str());

            LocalMessageT msg = {};
            msg.addr = addr;
            msg.type = LocalMessageType::SUB_DATA;
            msg.topic = topic;
            msg.payload = payload;

            return this->sendLocal(msg);
        }

        /**
         * @brief Subscribe DB timer tick callback
         *
         * Decrements subscribe database lifetimes.
         * Unsubscribes from unused topics.
         */
        void subDBTick()
        {
            SPSP_LOGD("SubDB: Tick running");

            this->subDBDecrementLifetimes();
            this->subDBRemoveExpiredEntries();
            this->subDBRemoveUnusedTopics();

            SPSP_LOGD("SubDB: Tick done");
        }

        /**
         * @brief Decrements lifetimes of entries
         *
         */
        void subDBDecrementLifetimes()
        {
            const std::scoped_lock lock(m_mutex);

            m_subDB.forEach([this] (const std::string& topic,
                                    const SubDBMapT& entryMap) {
                for (auto& [addr, entry] : entryMap) {
                    // Don't decrement entries with infinite lifetime
                    if (entry.lifetime != BRIDGE_SUB_NO_EXPIRE) {
                        m_subDB[topic][addr].lifetime--;
                    }
                }
            });
        }

        /**
         * @brief Removes expired entries
         *
         */
        void subDBRemoveExpiredEntries()
        {
            const std::scoped_lock lock(m_mutex);

            m_subDB.forEach([this] (const std::string& topic,
                                    const SubDBMapT& entryMap) {
                auto entryIt = entryMap.begin();
                while (entryIt != entryMap.end()) {
                    auto addr = entryIt->first;

                    if (entryIt->second.lifetime == 0) {
                        // Expired
                        entryIt = m_subDB[topic].erase(entryIt);
                        SPSP_LOGD("SubDB: Removed addr %s from topic '%s'",
                                  addr.str.c_str(), topic.c_str());
                    } else {
                        // Continue
                        entryIt++;
                    }
                }
            });
        }

        /**
         * @brief Removes and unsubscribes from unused topics
         *
         */
        void subDBRemoveUnusedTopics()
        {
            const std::scoped_lock lock(m_mutex);

            std::vector<std::string> unusedTopics;

            // Get unused topics
            m_subDB.forEach([&unusedTopics] (const std::string& topic,
                                             const SubDBMapT& entryMap) {
                if (entryMap.empty()) {
                    unusedTopics.push_back(topic);
                }
            });

            // Unsubscribe from them
            for (auto& topic : unusedTopics) {
                if (this->getFarLayer()->unsubscribe(topic)) {
                    // Unsub successful, remove topic from sub DB
                    m_subDB.remove(topic);
                    SPSP_LOGD("SubDB: Removed unused topic '%s'", topic.c_str());
                } else {
                    SPSP_LOGE("SubDB: Topic '%s' can't be unsubscribed. Will try again in next tick.",
                              topic.c_str());
                }
            }
        }
    };
} // namespace SPSP::Nodes

#undef SPSP_LOG_TAG
