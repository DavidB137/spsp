/**
 * @file spsp_espnow_adapter.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief ESP-NOW adapter for testing
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <functional>
#include <string>

#include "spsp_espnow_adapter_if.hpp"

namespace SPSP::LocalLayers::ESPNOW
{
    /**
     * @brief ESP-NOW adapter for testing
     *
     */
    class Adapter : public IAdapter
    {
    public:
        void setRecvCb(AdapterRecvCb cb) noexcept {}
        void setSendCb(AdapterSendCb cb) noexcept {}
        void send(const LocalAddrT& dst, const std::string& data) const {}
        void addPeer(const LocalAddrT& peer) {}
        void removePeer(const LocalAddrT& peer) {}
    };
} // namespace SPSP::LocalLayers::ESPNOW
