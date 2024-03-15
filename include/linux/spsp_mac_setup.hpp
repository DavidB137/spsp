/**
 * @file spsp_mac_setup.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Setup of MAC address
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

namespace SPSP
{
    /**
     * @brief Sets local MAC address
     *
     * If `buffer` is nullptr, doesn't do anything.
     *
     * @param mac Buffer (`MAC_LEN` bytes long)
     */
    void setLocalMAC(uint8_t *mac);
} // namespace SPSP
