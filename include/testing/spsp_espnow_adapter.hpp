/**
 * @file spsp_espnow_adapter.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief ESP-NOW adapter for testing
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <string>
#include <unordered_set>

#include "spsp_espnow_adapter_if.hpp"
#include "spsp_espnow_types.hpp"

namespace SPSP::LocalLayers::ESPNOW
{
    /**
     * @brief ESP-NOW adapter for testing
     *
     */
    class Adapter : public IAdapter
    {
        AdapterRecvCb m_recvCb = nullptr;
        AdapterSendCb m_sendCb = nullptr;
        std::unordered_set<LocalAddrT> m_peers;

    public:
        /**
         * @brief Sets receive callback
         *
         * Callback should be called in new thread.
         *
         * @param cb Callback
         */
        void setRecvCb(AdapterRecvCb cb) noexcept
        {
            m_recvCb = cb;
        }

        /**
         * @brief Gets receive callback
         *
         * @param cb Callback
         */
        AdapterRecvCb getRecvCb() const noexcept
        {
            return m_recvCb;
        }

        /**
         * @brief Sets send callback
         *
         * @param cb Callback
         */
        void setSendCb(AdapterSendCb cb) noexcept
        {
            m_sendCb = cb;
        }

        /**
         * @brief Gets send callback
         *
         * @param cb Callback
         */
        AdapterSendCb getSendCb() const noexcept
        {
            return m_sendCb;
        }

        /**
         * @brief Sends local message
         *
         * @param dst Destination address
         * @param data Raw data to be sent
         * @throw AdapterError when call to send function fails
         *        (not when packet undelivered)
         */
        virtual void send(const LocalAddrT& dst, const std::string& data) const
        {
            std::thread t(this->getSendCb(), dst, true);
            t.detach();
        }

        /**
         * @brief Adds peer to peer list
         *
         * @param peer Peer address
         * @throw AdapterError when peer can't be added
         */
        void addPeer(const LocalAddrT& peer)
        {
            m_peers.insert(peer);
        }

        /**
         * @brief Removes peer from peer list
         *
         * @param peer Peer address
         * @throw AdapterError when peer can't be removed
         */
        void removePeer(const LocalAddrT& peer)
        {
            if (!m_peers.erase(peer)) {
                throw AdapterError("Can't remove non-existing peer");
            }
        }
    };
} // namespace SPSP::LocalLayers::ESPNOW
