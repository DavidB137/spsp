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
#include "spsp_espnow.hpp"
#include "spsp_layers.hpp"
#include "spsp_local_addr.hpp"
#include "spsp_node.hpp"

namespace SPSP::Nodes
{
    /**
     * @brief Client node
     * 
     */
    class Client : public SPSP::ILocalNode<LocalLayers::ESPNOW::Layer>
    {
        // Subscribe DB can access private members
        friend class ClientSubDB;

        ClientSubDB m_subDB;
        std::mutex m_mutex;  //!< Mutex to prevent race conditions

    public:
        /**
         * @brief Constructs a new client node
         * 
         */
        Client();

        /**
         * @brief Destroys the client node
         * 
         */
        ~Client();

        /**
         * @brief Receives the message from far layer
         * 
         * This should not be used!
         * 
         * @param topic Topic
         * @param payload Payload (data)
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool receiveFar(const std::string topic, const std::string payload)
        {
            throw std::logic_error("You must not use far layer on client node!");
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
        bool publish(const std::string topic, const std::string payload);

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
        bool subscribe(const std::string topic, SubscribeCb cb);

        /**
         * @brief Resubscribes to all topics
         * 
         * Doesn't do anything.
         */
        void resubscribeAll() {}

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
         * @brief Predicate whether this node is a client
         * 
         * @return true This is a client
         * @return false This is not a client
         */
        inline bool isClient() { return true; }

    protected:
        /**
         * @brief Processes PROBE_REQ message
         * 
         * Doesn't do anything.
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processProbeReq(const LocalMessage<LocalAddr> req) { return false; }

        /**
         * @brief Processes PROBE_RES message
         * 
         * Doesn't do anything.
         * This is handled internally by concrete local layer.
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processProbeRes(const LocalMessage<LocalAddr> req) { return true; }

        /**
         * @brief Processes PUB message
         * 
         * Doesn't do anything.
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processPub(const LocalMessage<LocalAddr> req) { return false; }

        /**
         * @brief Processes SUB_REQ message
         * 
         * Doesn't do anything.
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processSubReq(const LocalMessage<LocalAddr> req) { return false; }

        /**
         * @brief Processes SUB_DATA message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processSubData(const LocalMessage<LocalAddr> req);

        /**
         * @brief Processes UNSUB message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processUnsub(const LocalMessage<LocalAddr> req) { return false; }
    };
} // namespace SPSP::Nodes
