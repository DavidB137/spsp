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

#include "spsp_bridge_sub_db.hpp"
#include "spsp_local_addr_mac.hpp"
#include "spsp_logger.hpp"
#include "spsp_node.hpp"

// Log tag
#define SPSP_LOG_TAG "SPSP/Bridge"

namespace SPSP::Nodes
{
    /**
     * @brief Bridge node
     * 
     * @tparam TLocalLayer Type of local layer
     * @tparam TFarLayer   Type of far layer
     */
    template <typename TLocalLayer, typename TFarLayer>
    class Bridge : public ILocalAndFarNode<TLocalLayer, TFarLayer>
    {
        // Subscribe DB can access private members
        friend class BridgeSubDB<TLocalLayer, TFarLayer>;

    protected:
        BridgeSubDB<TLocalLayer, TFarLayer> m_subDB;
        std::mutex m_mutex;  //!< Mutex to prevent race conditions

    public:
        using LocalAddrT = TLocalLayer::LocalAddrT;
        using LocalMessageT = TLocalLayer::LocalMessageT;

        /**
         * @brief Constructs a new bridge node
         * 
         */
        Bridge(TLocalLayer* ll, TFarLayer* fl)
            : ILocalAndFarNode<TLocalLayer, TFarLayer>{ll, fl}, m_subDB{this}
        {
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
        bool receiveFar(const std::string topic, const std::string payload)
        {
            SPSP_LOGD("Received far msg: topic '%s', payload '%s'",
                      topic.c_str(), payload.c_str());

            m_subDB.callCbs(topic, payload);
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
        bool publish(const std::string topic, const std::string payload)
        {
            SPSP_LOGD("Publishing locally: topic '%s', payload '%s'",
                      topic.c_str(), payload.c_str());

            return this->getFarLayer()->publish(LocalAddrMAC::local().str, topic, payload);
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
        bool subscribe(const std::string topic, SubscribeCb cb)
        {
            SPSP_LOGD("Subscribing locally to topic '%s'", topic.c_str());

            return m_subDB.insert(topic, LocalAddrT{}, cb);
        }

        /**
         * @brief Resubscribes to all topics
         * 
         */
        void resubscribeAll() { m_subDB.resubscribeAll(); }

        /**
         * @brief Unsubscribes from topic
         * 
         * This is primary endpoint for unsubscribing locally on this node.
         * 
         * @param topic Topic
         * @return true Unsubscribe successful
         * @return false Unsubscribe failed
         */
        bool unsubscribe(const std::string topic)
        {
            SPSP_LOGD("Unsubscribing locally from topic '%s'", topic.c_str());

            m_subDB.remove(topic, LocalAddrT{});
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
        bool processProbeReq(const LocalMessageT req,
                             int rssi = NODE_RSSI_UNKNOWN)
        {
            LocalMessageT res = req;
            res.type = LocalMessageType::PROBE_RES;
            res.payload = SPSP::VERSION;
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
        bool processProbeRes(const LocalMessageT req,
                             int rssi = NODE_RSSI_UNKNOWN) { return false; }

        /**
         * @brief Processes PUB message
         * 
         * @param req Request message
         * @param rssi Received signal strength indicator (in dBm)
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processPub(const LocalMessageT req,
                        int rssi = NODE_RSSI_UNKNOWN)
        {
            return this->getFarLayer()->publish(req.addr.str, req.topic, req.payload);
        }

        /**
         * @brief Processes SUB_REQ message
         * 
         * @param req Request message
         * @param rssi Received signal strength indicator (in dBm)
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processSubReq(const LocalMessageT req,
                           int rssi = NODE_RSSI_UNKNOWN)
        {
            return m_subDB.insert(req.topic, req.addr, nullptr);
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
        bool processSubData(const LocalMessageT req,
                            int rssi = NODE_RSSI_UNKNOWN) { return false; }

        /**
         * @brief Processes UNSUB message
         * 
         * @param req Request message
         * @param rssi Received signal strength indicator (in dBm)
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processUnsub(const LocalMessageT req,
                          int rssi = NODE_RSSI_UNKNOWN)
        {
            m_subDB.remove(req.topic, req.addr);
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
        bool publishSubData(const LocalAddrT addr, const std::string topic,
                            const std::string payload)
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
         * @brief Subscribes to topic on far layer (if connected)
         * 
         * @param topic Topic
         * @return true Subscription successful
         * @return false Subscription failed
         */
        bool subscribeFar(const std::string topic)
        {
            return this->getFarLayer()->subscribe(topic);
        }

        /**
         * @brief Unsubscribes from topic on far layer (if connected)
         * 
         * @param topic Topic
         * @return true Unsubscription successful
         * @return false Unsubscription failed
         */
        bool unsubscribeFar(const std::string topic)
        {
            return this->getFarLayer()->unsubscribe(topic);
        }
    };
} // namespace SPSP::Nodes

#undef SPSP_LOG_TAG
