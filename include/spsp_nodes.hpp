/**
 * @file spsp_nodes.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Node types of SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include "spsp_interfaces.hpp"

namespace SPSP::Nodes
{
    /**
     * @brief Client node
     * 
     */
    class Client : public SPSP::INode
    {
    public:
        using SPSP::INode::INode;

        /**
         * @brief Receives the message from local layer
         * 
         * Acts as a callback for local layer receiver.
         * 
         * @param msg Received message
         */
        void receiveLocal(Message msg);
    };

    /**
     * @brief Bridge node
     * 
     */
    class Bridge : public SPSP::INode
    {
    protected:
        SPSP::IFarLayer* m_fl = nullptr;
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
         * Safe to call even when far layer is already set.
         * 
         * @param fl New far layer
         */
        void setFarLayer(IFarLayer* fl)
        {
            // Unset old far layer
            if (m_fl != nullptr) this->unsetFarLayer();

            m_fl = fl;
            m_fl->setNode(this);
        }

        /**
         * @brief Unsets pointer to the far layer.
         * 
         */
        void unsetFarLayer()
        {
            if (m_fl != nullptr) {
                m_fl->unsetNode();
                m_fl = nullptr;
            }
        }

        /**
         * @brief Checks whether the far layer is connected
         * 
         * @return true Far layer is connected
         * @return false Far layer is disconnected
         */
        inline bool farLayerConnected() const { return m_fl != nullptr; }

        /**
         * @brief Receives the message from local layer
         * 
         * Acts as a callback for local layer receiver.
         * 
         * @param msg Received message
         */
        void receiveLocal(Message msg);

        /**
         * @brief Receives the message from far layer
         * 
         * Acts as a callback for far layer receiver.
         * 
         * @param msg Received message
         */
        void receiveFar(Message msg);
    };

    /**
     * @brief Client and bridge node
     * 
     */
    class ClientBridge : public Client, public Bridge
    {
    public:
        using Bridge::Bridge;

        /**
         * @brief Receives the message from local layer
         * 
         * Acts as a callback for local layer receiver.
         * 
         * @param msg Received message
         */
        void receiveLocal(Message msg);

        using Bridge::receiveFar;
    };
} // namespace SPSP::Nodes
