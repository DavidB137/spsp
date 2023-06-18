/**
 * @file spsp_bridge.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Bridge node type of SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include "spsp_client.hpp"
#include "spsp_layers.hpp"
#include "spsp_node.hpp"

namespace SPSP::Nodes
{
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
         * @brief Receives the message from far layer
         * 
         * Acts as a callback for far layer receiver.
         * 
         * @param topic Topic
         * @param payload Payload (data)
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool receiveFar(const std::string topic, const std::string payload);

    protected:
        /**
         * @brief Processes PONG message
         * 
         * Doesn't do anything.
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processPong(const Message req) { return true; }

        /**
         * @brief Processes PUB message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processPub(const Message req);

        /**
         * @brief Processes SUB_REQ message
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processSubReq(const Message req);

        /**
         * @brief Processes SUB_DATA message
         * 
         * Doesn't do anything.
         * 
         * @param req Request message
         * @return true Message delivery successful
         * @return false Message delivery failed
         */
        bool processSubData(const Message req) { return true; }
    };
} // namespace SPSP::Nodes
