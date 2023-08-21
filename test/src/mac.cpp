/**
 * @file spsp_mac.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief MAC address
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "esp_mac.h"

#include "spsp_mac.hpp"
#include "spsp_random.hpp"

namespace SPSP
{
    void getLocalMAC(uint8_t *mac)
    {
        if (mac != nullptr) {
            Random rand;
            rand.bytes(mac, MAC_LEN);
        }
    }
} // namespace SPSP
