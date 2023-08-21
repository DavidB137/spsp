/**
 * @file spsp_layers.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Local and far layers for SPSP
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include "spsp_local_message.hpp"

namespace SPSP
{
    // Forward declaration
    template <typename TLocalLayer> class ILocalNode;
    template <typename TFarLayer> class IFarNode;

    /**
     * @brief Interface for local layer
     *
     * @tparam TLocalMessage Type of local message
     */
    template <typename TLocalMessage>
    class ILocalLayer
    {
        void* m_node = nullptr;

    public:
        using LocalAddrT = typename TLocalMessage::LocalAddrT;
        using LocalMessageT = TLocalMessage;

        /**
         * @brief Sets pointer to the owner node.
         *
         * @param n Owner node
         */
        void setNode(void* n)
        {
            m_node = n;
        }

        /**
         * @brief Gets the node object
         *
         * @return Node pointer
         */
        inline ILocalNode<ILocalLayer>* getNode() const
        {
            return static_cast<ILocalNode<ILocalLayer>*>(m_node);
        }

        /**
         * @brief Checks whether the owner node is connected
         *
         * @return true Node is connected
         * @return false Node is disconnected
         */
        inline bool nodeConnected() const
        {
            return m_node != nullptr;
        }

        /**
         * @brief Sends the message to given node
         *
         * Should be used by `INode` only!
         *
         * In the message, empty address means send to the bridge peer.
         *
         * @param msg Message
         * @return true Delivery successful
         * @return false Delivery failed
         */
        virtual bool send(const TLocalMessage& msg) = 0;
    };

    /**
     * @brief Interface for far layer
     *
     */
    class IFarLayer
    {
        void* m_node = nullptr;

    public:
        /**
         * @brief Sets pointer to the owner node.
         *
         * @param n Owner node
         */
        void setNode(void* n)
        {
            m_node = n;
        }

        /**
         * @brief Gets the node object
         *
         * @return Node pointer
         */
        inline IFarNode<IFarLayer>* getNode() const
        {
            return static_cast<IFarNode<IFarLayer>*>(m_node);
        }

        /**
         * @brief Checks whether the owner node is connected
         *
         * @return true Node is connected
         * @return false Node is disconnected
         */
        inline bool nodeConnected() const
        {
            return m_node != nullptr;
        }

        /**
         * @brief Publishes message coming from node
         *
         * Should be used by `INode` only!
         *
         * @param src Source address
         * @param topic Topic
         * @param payload Payload (data)
         * @return true Delivery successful
         * @return false Delivery failed
         */
        virtual bool publish(const std::string& src, const std::string& topic,
                             const std::string& payload) = 0;

        /**
         * @brief Subscribes to given topic
         *
         * Should be used by `INode` only!
         *
         * @param topic Topic
         * @return true Subscribe successful
         * @return false Subscribe failed
         */
        virtual bool subscribe(const std::string& topic) = 0;

         /**
         * @brief Unsubscribes from given topic
         *
         * Should be used by `INode` only!
         *
         * @param topic Topic
         * @return true Unsubscribe successful
         * @return false Unsubscribe failed
         */
        virtual bool unsubscribe(const std::string& topic) = 0;
    };
} // namespace SPSP
