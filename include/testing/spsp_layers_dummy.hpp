/**
 * @file spsp_layers_dummy.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Dummy layers for testing
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

#include "spsp_layers.hpp"
#include "spsp_local_addr.hpp"
#include "spsp_local_message.hpp"

namespace SPSP::LocalLayers
{
    /**
     * @brief Dummy local layer for testing
     *
     */
    class DummyLocalLayer : public ILocalLayer<LocalMessage<LocalAddr>>
    {
    public:
        using LocalAddrT = SPSP::LocalAddr;
        using LocalMessageT = SPSP::LocalMessage<SPSP::LocalAddr>;
        using SentMsgsSetT = std::unordered_set<LocalMessageT>;

    protected:
        SentMsgsSetT m_sentMsgs;
        size_t m_sentMsgsCount = 0;

    public:
        /**
         * @brief Sends the message to given node
         *
         * In the message, empty address means send to the bridge peer.
         *
         * @param msg Message
         * @return true Delivery successful
         * @return false Delivery failed
         */
        virtual bool send(const LocalMessageT& msg)
        {
            m_sentMsgs.insert(msg);
            m_sentMsgsCount++;
            return true;
        }

        /**
         * @brief Simulates reception of data from local layer
         *
         * @param msg Message
         * @param rssi Received signal strength indicator (in dBm)
         * @return true Delivery successful
         * @return false Delivery failed
         */
        void receiveDirect(const LocalMessageT& msg, int rssi = 0)
        {
            if (this->nodeConnected()) {
                this->getNode()->receiveLocal(msg, rssi);
            }
        }

        /**
         * @brief Returns set of sent messages
         *
         * @return Sent messages set
         */
        const SentMsgsSetT& getSentMsgs()
        {
            return m_sentMsgs;
        }

        /**
         * @brief Returns count of sent messages
         *
         * @return Sent messages count
         */
        size_t getSentMsgsCount()
        {
            return m_sentMsgsCount;
        }
    };
} // namespace SPSP::LocalLayers

namespace SPSP::FarLayers
{
    /**
     * @brief Dummy far layer for testing
     *
     */
    class DummyFarLayer : public IFarLayer
    {
    public:
        using PubsSetT = std::unordered_set<std::string>;  // not using tuple, because plain string if simpler
        using SubsSetT = std::unordered_set<std::string>;
        using SubsLogT = std::vector<std::string>;

    protected:
        PubsSetT m_pubs;
        SubsSetT m_subs;
        SubsLogT m_subsLog;
        SubsLogT m_unsubsLog;

    public:
        /**
         * @brief Publishes message coming from node
         *
         * @param src Source address
         * @param topic Topic
         * @param payload Payload (data)
         * @return true Delivery successful
         * @return false Delivery failed
         */
        virtual bool publish(const std::string& src, const std::string& topic,
                             const std::string& payload)
        {
            // `LocalMessage.toString()`-compatible string
            m_pubs.insert("PUB " + src + " " + topic + " " + payload);
            return true;
        }

        /**
         * @brief Subscribes to given topic
         *
         * @param topic Topic
         * @return true Subscribe successful
         * @return false Subscribe failed
         */
        virtual bool subscribe(const std::string& topic)
        {
            m_subs.insert(topic);
            m_subsLog.push_back(topic);
            return true;
        }

         /**
         * @brief Unsubscribes from given topic
         *
         * @param topic Topic
         * @return true Unsubscribe successful
         * @return false Unsubscribe failed
         */
        virtual bool unsubscribe(const std::string& topic)
        {
            m_unsubsLog.push_back(topic);
            return m_subs.erase(topic);
        }

        /**
         * @brief Simulates reception of data from far layer
         *
         * @param topic Topic
         * @param payload Payload (data)
         */
        virtual void receiveDirect(const std::string& topic,
                                   const std::string& payload)
        {
            if (this->nodeConnected()) {
                this->getNode()->receiveFar(topic, payload);
            }
        }

        /**
         * @brief Returns current publishes set
         *
         * @return Publishes set
         */
        const PubsSetT& getPubs()
        {
            return m_pubs;
        }

        /**
         * @brief Returns current subscriptions set
         *
         * @return Subscriptions set
         */
        const SubsSetT& getSubs()
        {
            return m_subs;
        }

        /**
         * @brief Returns current subscriptions log
         *
         * @return Subscriptions log
         */
        const SubsLogT& getSubsLog()
        {
            return m_subsLog;
        }

        /**
         * @brief Returns current unsubscriptions log
         *
         * @return Unsubscriptions log
         */
        const SubsLogT& getUnsubsLog()
        {
            return m_unsubsLog;
        }
    };
} // namespace SPSP::FarLayers
