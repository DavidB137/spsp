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

#include "spsp_espnow_types.hpp"

namespace SPSP::LocalLayers::ESPNOW
{
    // Callback types
    using AdapterRecvCb = std::function<void(const LocalMessageT msg)>;
    using AdapterSendCb = std::function<void(const LocalAddrT dst, bool delivered)>;

    class IAdapter
    {
    public:
        // Do initialization in constructor
        // and deinitialization in destructor

        /**
         * @brief Sets receive callback
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
         * @param msg Message
         */
        virtual void send(const LocalMessageT& msg) = 0;

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
