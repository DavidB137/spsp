/**
 * @file spsp_mac.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief MAC address
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <cstdint>
#include <cstdlib>

namespace SPSP
{
    /**
     * @brief Length of MAC address in bytes
     *
     * Should be the same as `ESP_NOW_ETH_ALEN`, but plaform independent.
     */
    static constexpr size_t MAC_LEN = 6;

    /**
     * @brief Gets local MAC address
     *
     * If `buffer` is nullptr, doesn't do anything.
     *
     * @param mac Buffer (`MAC_LEN` bytes long)
     */
    void getLocalMAC(uint8_t *mac);
} // namespace SPSP
