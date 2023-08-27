/**
 * @file spsp_client.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Client node type of SPSP
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <mutex>

#include "spsp_client_sub_db.hpp"
#include "spsp_logger.hpp"
#include "spsp_node.hpp"

// Log tag
#define SPSP_LOG_TAG "SPSP/Client"

namespace SPSP::Nodes
{
    /**
     * @brief Client node
     *
     * @tparam TLocalLayer Type of local layer
     */
    template <typename TLocalLayer>
    class Client : public ILocalNode<TLocalLayer>
    {
        // Subscribe DB can access private members
        friend class ClientSubDB<TLocalLayer>;

    protected:
        ClientSubDB<TLocalLayer> m_subDB;
        std::mutex m_mutex;  //!< Mutex to prevent race conditions

    public:
        using LocalAddrT = TLocalLayer::LocalAddrT;
        using LocalMessageT = TLocalLayer::LocalMessageT;

        /**
         * @brief Constructs a new client node
         *
         * @param ll Local layer
         */
        Client(TLocalLayer* ll) : ILocalNode<TLocalLayer>{ll}, m_subDB{this}
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

            LocalMessageT msg = {};
            // msg.addr is empty => send to the bridge node
            msg.type = LocalMessageType::SUB_REQ;
            msg.topic = topic;
            // msg.payload is empty

            // Add to sub DB
            m_subDB.insert(topic, cb);

            return this->sendLocal(msg);
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

            return this->sendLocal(msg);
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
            m_subDB.callCb(req.topic, req.payload);
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
    };
} // namespace SPSP::Nodes

#undef SPSP_LOG_TAG
