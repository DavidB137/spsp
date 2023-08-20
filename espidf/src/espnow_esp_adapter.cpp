/**
 * @file espnow_esp_adapter.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief ESP-NOW adapter for ESP platform
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <thread>

#include "esp_mac.h"
#include "esp_now.h"
#include "esp_random.h"
#include "esp_wifi.h"

#include "spsp_espnow_esp_adapter.hpp"
#include "spsp_exception_check.hpp"

// Log tag
static const char* SPSP_LOG_TAG = "SPSP/Local/ESPNOW";

namespace SPSP::LocalLayers::ESPNOW
{
    // Instance pointer
    // ESP-NOW callbacks don't take `void*` context pointers, so we have to get
    // creative.
    static Adapter* _adapterInstance = nullptr;

    // Wrapper for C receive callback
    void _receiveCallback(const esp_now_recv_info_t* espnowInfo, const uint8_t* data, int dataLen)
    {
        auto cb = _adapterInstance->getRecvCb();
        auto rssi = espnowInfo->rx_ctrl->rssi;

        // Create new thread for receive handler
        // Otherwise creates deadlock, because receive callback tries to send
        // response, but ESP-NOW's internal mutex is still held by this
        // unfinished callback.
        std::thread t(cb, espnowInfo->src_addr,
                      std::string((char*) data, dataLen), rssi);

        // Run independently
        t.detach();
    }

    // Wrapper for C send callback
    void _sendCallback(const uint8_t *dst, esp_now_send_status_t status)
    {
        auto cb = _adapterInstance->getSendCb();
        cb(dst, status == ESP_NOW_SEND_SUCCESS);
    }

    Adapter::Adapter()
    {
        SPSP_ERROR_CHECK(esp_now_init(),
                         AdapterError("Initialization failed"));
        SPSP_ERROR_CHECK(esp_now_register_recv_cb(_receiveCallback),
                         AdapterError("Internal receive callback registration failed"));
        SPSP_ERROR_CHECK(esp_now_register_send_cb(_sendCallback),
                         AdapterError("Internal send callback registration failed"));
    }

    Adapter::~Adapter()
    {
        esp_now_deinit();
    }

    void Adapter::setRecvCb(AdapterRecvCb cb)
    {
        m_recvCb = cb;
    }

    AdapterRecvCb Adapter::getRecvCb() const
    {
        return m_recvCb;
    }

    void Adapter::setSendCb(AdapterSendCb cb)
    {
        m_sendCb = cb;
    }

    AdapterSendCb Adapter::getSendCb() const
    {
        return m_sendCb;
    }

    void Adapter::send(const LocalAddrT& dst, const std::string& data) const
    {
        // Get MAC address
        esp_now_peer_info_t peerInfo = {};
        dst.toMAC(peerInfo.peer_addr);

        SPSP_ERROR_CHECK(esp_now_send(peerInfo.peer_addr,
                                      (const uint8_t*) data.c_str(),
                                      data.length()),
                         AdapterError("Sending failed"));
    }

    void Adapter::addPeer(const LocalAddrT& peer)
    {
        // Get MAC address
        esp_now_peer_info_t peerInfo = {};
        peer.toMAC(peerInfo.peer_addr);

        SPSP_ERROR_CHECK(esp_now_add_peer(&peerInfo),
                         AdapterError("Adding peer failed"));
    }

    void Adapter::removePeer(const LocalAddrT& peer)
    {
        // Get MAC address
        esp_now_peer_info_t peerInfo = {};
        peer.toMAC(peerInfo.peer_addr);

        SPSP_ERROR_CHECK(esp_now_del_peer(peerInfo.peer_addr),
                         AdapterError("Deleting peer failed"));
    }
} // namespace SPSP::LocalLayers::ESPNOW
