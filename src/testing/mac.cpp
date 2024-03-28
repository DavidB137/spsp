/**
 * @file mac.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief MAC address
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "spsp/mac.hpp"
#include "spsp/random.hpp"

namespace SPSP
{
    void getLocalMAC(uint8_t *mac)
    {
        if (mac != nullptr) {
            mac[0] = 1;
            mac[1] = 2;
            mac[2] = 3;
            mac[3] = 4;
            mac[4] = 5;
            mac[5] = 6;
        }
    }
} // namespace SPSP
