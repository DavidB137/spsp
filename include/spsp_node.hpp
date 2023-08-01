/**
 * @file spsp_node.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Node interface for SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <climits>
#include <memory>
#include <mutex>

#include "spsp_layers.hpp"
#include "spsp_local_message.hpp"
#include "spsp_logger.hpp"

// Log tag
#define SPSP_LOG_TAG "SPSP/Node"

namespace SPSP
{
    /**
     * @brief Subscribe callback type
     * 
     */
    using SubscribeCb = void (*)(const std::string topic,
                                 const std::string payload);

    /**
     * @brief Most generic node type of SPSP
     * 
     * Doesn't do anything at all.
     */
    class INode
    {
    public:
        /**
         * @brief Publishes payload to topic
         * 
         * This is primary endpoint for publishing data locally on all node types.
         * 
         * @param topic Topic
         * @param payload Payload
         * @return true Delivery successful
         * @return false Delivery failed
         */
        virtual bool publish(const std::string topic, const std::string payload) = 0;

        /**
         * @brief Subscribes to topic
         * 
         * This is primary endpoint for subscribing locally on all node types.
         * 
         * @param topic Topic
         * @param cb Callback function
         * @return true Subscribe successful
         * @return false Subscribe failed
         */
        virtual bool subscribe(const std::string topic, SubscribeCb cb) = 0;

        /**
         * @brief Unsubscribes from topic
         * 
         * This is primary endpoint for unsubscribing locally on all node types.
         * 
         * @param topic Topic
         * @return true Unsubscribe successful
         * @return false Unsubscribe failed
         */
        virtual bool unsubscribe(const std::string topic) = 0;
    };

    /**
     * @brief Generic local node of SPSP
     * 
     * @tparam TLocalLayer Type of local layer
     */
    template <typename TLocalLayer>
    class ILocalNode : public INode
    {
        using LocalAddrT = typename TLocalLayer::LocalAddrT;
        using LocalMessageT = typename TLocalLayer::LocalMessageT;
        using LocalRecvSendCb = void (*)(const LocalMessageT&);

        TLocalLayer* m_ll;
        LocalRecvSendCb m_localRecvSendCb = nullptr;

    public:
        /**
         * @brief Constructs a new node
         * 
         */
        ILocalNode(TLocalLayer* ll) : m_ll{ll}
        {
            m_ll->setNode(this);
        }

        /**
         * @brief Receives the message from local layer
         * 
         * Acts as a callback for local layer receiver.
         * 
         * @param msg Received message
         * @param rssi Received signal strength indicator (in dBm)
         */
        void receiveLocal(const LocalMessageT msg, int rssi = INT_MAX)
        {
            if (rssi < INT_MAX) {
                SPSP_LOGI("Received local msg: %s (%d dBm)",
                          msg.toString().c_str(), rssi);
            } else {
                SPSP_LOGI("Received local msg: %s", msg.toString().c_str());
            }

            // Call receive/send callback
            if (m_localRecvSendCb != nullptr) {
                SPSP_LOGD("Calling receive/send callback %p",
                          m_localRecvSendCb);
                m_localRecvSendCb(msg);
            }

            bool processed = false;

            // Call responsible handler
            switch (msg.type) {
            case LocalMessageType::PROBE_REQ: processed = processProbeReq(msg); break;
            case LocalMessageType::PROBE_RES: processed = processProbeRes(msg); break;
            case LocalMessageType::PUB: processed = processPub(msg); break;
            case LocalMessageType::SUB_REQ: processed = processSubReq(msg); break;
            case LocalMessageType::SUB_DATA: processed = processSubData(msg); break;
            case LocalMessageType::UNSUB: processed = processUnsub(msg); break;
            default:
                SPSP_LOGW("Unprocessable message type %s (%d)",
                        localMessageTypeToStr(msg.type),
                        static_cast<uint8_t>(msg.type));
                break;
            }

            if (processed) {
                SPSP_LOGI("Message processed: %s", msg.toString().c_str());
            } else {
                SPSP_LOGE("Message not processed: %s", msg.toString().c_str());
            }
        }

        /**
         * @brief Sets local receive/send callback function
         * 
         * May be used to blink LEDs, compute statistics, etc.
         * 
         * Don't do any long action inside the callback!
         * If you need to perform long blocking operation, spawn yourself
         * a new thread.
         * 
         * @param cb Callback (if `nullptr`, unsets the callback)
         */
        void setLocalRecvSendCb(LocalRecvSendCb cb)
        {
            m_localRecvSendCb = cb;
        }

    protected:
        /**
         * @brief Gets the far layer object
         * 
         * @return Far layer object
         */
        inline TLocalLayer* getLocalLayer() const
        {
            return m_ll;
        }

        /**
         * @brief Sends the message to local layer
         * 
         * @param msg Message to send
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool sendLocal(const LocalMessageT msg)
        {
            SPSP_LOGI("Sending local msg: %s", msg.toString().c_str());

            // Send to local layer
            bool delivered = m_ll->send(msg);

            if (delivered) {
                SPSP_LOGI("Message delivered: %s", msg.toString().c_str());
            } else {
                SPSP_LOGE("Message not delivered: %s", msg.toString().c_str());
            }

            // Call receive/send callback
            if (m_localRecvSendCb != nullptr) {
                SPSP_LOGD("Calling receive/send callback %p",
                          m_localRecvSendCb);
                m_localRecvSendCb(msg);
            }

            return delivered;
        }

        /**
         * @brief Processes PROBE_REQ message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        virtual bool processProbeReq(const LocalMessageT req) = 0;

        /**
         * @brief Processes PROBE_RES message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        virtual bool processProbeRes(const LocalMessageT req) = 0;

        /**
         * @brief Processes PUB message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        virtual bool processPub(const LocalMessageT req) = 0;

        /**
         * @brief Processes SUB_REQ message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        virtual bool processSubReq(const LocalMessageT req) = 0;

        /**
         * @brief Processes SUB_DATA message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        virtual bool processSubData(const LocalMessageT req) = 0;

        /**
         * @brief Processes UNSUB message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        virtual bool processUnsub(const LocalMessageT req) = 0;
    };

    /**
     * @brief Generic far layer node of SPSP
     * 
     * Supplied far layer must be valid during whole lifetime of this
     * 
     * @tparam TFarLayer Type of far layer
     */
    template <typename TFarLayer>
    class IFarNode : public INode
    {
        TFarLayer* m_fl;

    public:
        /**
         * @brief Constructs a new node
         * 
         */
        IFarNode(TFarLayer* fl) : m_fl{fl}
        {
            m_fl->setNode(this);
        }

        /**
         * @brief Receives the message from far layer
         * 
         * Acts as a callback for far layer receiver.
         * On client node: throws error.
         * 
         * This is defined here just because MQTT's `getNode()->receiveFar()`.
         * TODO: think of better solution
         * 
         * @param topic Topic
         * @param payload Payload (data)
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        virtual bool receiveFar(const std::string topic, const std::string payload) = 0;

        /**
         * @brief Resubscribes to all topics
         * 
         */
        virtual void resubscribeAll() = 0;
    
    protected:
        /**
         * @brief Gets the far layer object
         * 
         * @return Far layer object
         */
        inline TFarLayer* getFarLayer() const { return m_fl; }
    };

    /**
     * @brief Generic local and far layer node of SPSP
     * 
     * @tparam TLocalLayer Type of local layer
     * @tparam TFarLayer Type of far layer
     */
    template<typename TLocalLayer, typename TFarLayer>
    class ILocalAndFarNode : public ILocalNode<TLocalLayer>,
                             public IFarNode<TFarLayer>
    {
    public:
        /**
         * @brief Constructs a new node
         * 
         */
        ILocalAndFarNode(TLocalLayer *ll, TFarLayer *fl)
            : ILocalNode<TLocalLayer>(ll), IFarNode<TFarLayer>(fl) {}
    };
} // namespace SPSP

#undef SPSP_LOG_TAG
