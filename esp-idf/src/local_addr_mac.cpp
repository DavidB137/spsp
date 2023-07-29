/**
 * @file local_addr_mac.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Local layer address container for MAC address
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <cstring>

#include "esp_mac.h"

#include "spsp_local_addr_mac.hpp"

namespace SPSP
{
    LocalAddrMAC::LocalAddrMAC(const uint8_t* mac)
    {
        // Internal representation
        addr = std::vector<uint8_t>(mac, mac + MAC_LEN);

        // Printable string
        char macStr[2*MAC_LEN + 1];
        sprintf(macStr, "%02x%02x%02x%02x%02x%02x", MAC2STR(mac));
        str = std::string(macStr);
    }

    LocalAddrMAC LocalAddrMAC::local()
    {
        uint8_t mac[MAC_LEN];
        ESP_ERROR_CHECK(esp_efuse_mac_get_default(mac));
        return LocalAddrMAC(mac);
    }

    LocalAddrMAC LocalAddrMAC::zeroes()
    {
        uint8_t mac[MAC_LEN];
        memset(mac, 0x00, MAC_LEN);
        return LocalAddrMAC(mac);
    }

    LocalAddrMAC LocalAddrMAC::broadcast()
    {
        uint8_t mac[MAC_LEN];
        memset(mac, 0xFF, MAC_LEN);
        return LocalAddrMAC(mac);
    }

    void LocalAddrMAC::toMAC(uint8_t* mac) const
    {
        for (unsigned i = 0; i < addr.size(); i++) {
            mac[i] = addr[i];
        }
    }
}
