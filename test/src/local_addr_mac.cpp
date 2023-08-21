/**
 * @file local_addr_mac.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Local layer address container for MAC address
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <cstring>
#include <memory>

#include "spsp_local_addr_mac.hpp"

namespace SPSP
{
    LocalAddrMAC::LocalAddrMAC()
    {
        uint8_t mac[MAC_LEN];
        memset(mac, 0x00, MAC_LEN);
    }

    LocalAddrMAC::LocalAddrMAC(const uint8_t* mac)
    {
        // Internal representation
        addr = std::vector<uint8_t>(mac, mac + MAC_LEN);

        // Printable string
        char macStr[2*MAC_LEN + 1];
        sprintf(macStr, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        str = std::string(macStr);
    }

    LocalAddrMAC LocalAddrMAC::local()
    {
        return LocalAddrMAC::zeroes();
    }

    LocalAddrMAC LocalAddrMAC::zeroes()
    {
        return LocalAddrMAC();
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
