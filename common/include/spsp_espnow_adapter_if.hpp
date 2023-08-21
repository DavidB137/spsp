/**
 * @file spsp_espnow_adapter.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Interface for platform-dependent ESP-NOW adapter
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <functional>
#include <string>

#include "spsp_espnow_types.hpp"
#include "spsp_exception.hpp"

namespace SPSP::LocalLayers::ESPNOW
{
    /**
     * @brief Adapter error
     *
     */
    class AdapterError : public SPSP::Exception
    {
        using SPSP::Exception::Exception;
    };

    // Callback types
    using AdapterRecvCb = std::function<void(const LocalAddrT sndr, std::string data, int rssi)>;
    using AdapterSendCb = std::function<void(const LocalAddrT dst, bool delivered)>;

    /**
     * @brief Interface for platform-dependent ESP-NOW adapter
     *
     * Low level API for ESP-NOW communication.
     *
     * Each platform should implement this interface in class `Adapter`.
     */
    class IAdapter
    {
    public:
        /**
         * @brief Sets receive callback
         *
         * Callback should be called in new thread.
         *
         * @param cb Callback
         */
        virtual void setRecvCb(AdapterRecvCb cb) = 0;

        /**
         * @brief Sets send callback
         *
         * @param cb Callback
         */
        virtual void setSendCb(AdapterSendCb cb) = 0;

        /**
         * @brief Sends local message
         *
         * @param dst Destination address
         * @param data Raw data to be sent
         */
        virtual void send(const LocalAddrT& dst, const std::string& data) = 0;

        /**
         * @brief Adds peer to peer list
         *
         * @param peer Peer address
         */
        virtual void addPeer(const LocalAddrT& peer) = 0;

        /**
         * @brief Removes peer from peer list
         *
         * @param peer Peer address
         */
        virtual void removePeer(const LocalAddrT& peer) = 0;
    };
} // namespace SPSP::LocalLayers::ESPNOW
