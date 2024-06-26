/**
 * @file spsp_mac.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief MAC address
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "esp_mac.h"

#include "spsp/mac.hpp"

namespace SPSP
{
    void getLocalMAC(uint8_t *mac)
    {
        esp_efuse_mac_get_default(mac);
    }
} // namespace SPSP
