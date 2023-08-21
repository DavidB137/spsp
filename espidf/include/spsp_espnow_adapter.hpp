/**
 * @file spsp_espnow_adapter.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief ESP-NOW adapter for ESP platform
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <functional>
#include <string>

#include "spsp_espnow_adapter_if.hpp"
#include "spsp_espnow_types.hpp"

namespace SPSP::LocalLayers::ESPNOW
{
    /**
     * @brief ESP-NOW adapter for ESP platform
     *
     * Low level API for ESP-NOW communication.
     *
     * Implements `IAdapter` interface.
     */
    class Adapter : public IAdapter
    {
        AdapterRecvCb m_recvCb = nullptr;
        AdapterSendCb m_sendCb = nullptr;

    public:
        /**
         * @brief Constructs a new ESP-NOW adapter
         *
         * @throw AdapterError when any call to underlaying library fails
         */
        Adapter();

        /**
         * @brief Destroys the adapter
         *
         */
        ~Adapter();

        /**
         * @brief Sets receive callback
         *
         * Callback should be called in new thread.
         *
         * @param cb Callback
         */
        void setRecvCb(AdapterRecvCb cb) noexcept;

        /**
         * @brief Gets receive callback
         *
         * @param cb Callback
         */
        AdapterRecvCb getRecvCb() const noexcept;

        /**
         * @brief Sets send callback
         *
         * @param cb Callback
         */
        void setSendCb(AdapterSendCb cb) noexcept;

        /**
         * @brief Gets send callback
         *
         * @param cb Callback
         */
        AdapterSendCb getSendCb() const noexcept;

        /**
         * @brief Sends local message
         *
         * @param dst Destination address
         * @param data Raw data to be sent
         * @throw AdapterError when call to send function fails
         *        (not when packet undelivered)
         */
        void send(const LocalAddrT& dst, const std::string& data) const;

        /**
         * @brief Adds peer to peer list
         *
         * @param peer Peer address
         * @throw AdapterError when peer can't be added
         */
        void addPeer(const LocalAddrT& peer);

        /**
         * @brief Removes peer from peer list
         *
         * @param peer Peer address
         * @throw AdapterError when peer can't be removed
         */
        void removePeer(const LocalAddrT& peer);
    };
} // namespace SPSP::LocalLayers::ESPNOW
