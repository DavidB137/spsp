/**
 * @file spsp_mac.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief MAC address
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <cstring>

#include "spsp/mac.hpp"
#include "spsp/mac_setup.hpp"

namespace SPSP
{
    // Storage for local MAC
    static uint8_t LOCAL_MAC[MAC_LEN] = {};

    void getLocalMAC(uint8_t *mac)
    {
        if (mac != nullptr) {
            memcpy(mac, LOCAL_MAC, MAC_LEN);
        }
    }

    void setLocalMAC(uint8_t *mac)
    {
        if (mac != nullptr) {
            memcpy(LOCAL_MAC, mac, MAC_LEN);
        }
    }
} // namespace SPSP
