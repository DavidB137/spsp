/**
 * @file spsp_local_addr_mac.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Local layer address container for MAC address
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <string>
#include <vector>

#include "spsp_local_addr.hpp"

namespace SPSP
{
    /**
     * @brief Length of MAC address in bytes
     *
     * Should be the same as `ESP_NOW_ETH_ALEN`, but plaform independent.
     */
    constexpr const size_t MAC_LEN = 6;

    /**
     * @brief Local layer address container for MAC address
     *
     * MAC as addess type is very common, so implement everything here.
     */
    struct LocalAddrMAC : public LocalAddr
    {
        /**
         * @brief Construct a new object from 00:00:00:00:00:00 MAC address
         *
         */
        LocalAddrMAC();

        /**
         * @brief Constructs a new object
         *
         * @param mac MAC address
         */
        LocalAddrMAC(const uint8_t* mac);

        /**
         * @brief Constructs a new object from MAC address of this node
         *
         * @return New `LocalAddrMAC` object
         */
        static LocalAddrMAC local();

        /**
         * @brief Constructs a new object from 00:00:00:00:00:00 MAC address
         *
         * @return New `LocalAddrMAC` object
         */
        static LocalAddrMAC zeroes();

        /**
         * @brief Constructs a new object from broadcast MAC address
         *
         * @return New `LocalAddrMAC` object
         */
        static LocalAddrMAC broadcast();

        /**
         * @brief Converts `LocalAddrMAC` to MAC itself
         *
         * @param mac MAC address storage pointer
         */
        void toMAC(uint8_t* mac) const;
    };
}

// Define hasher function
template<>
struct std::hash<SPSP::LocalAddrMAC>
{
    std::size_t operator()(SPSP::LocalAddrMAC const& addr) const noexcept
    {
        return std::hash<SPSP::LocalAddr>{}(addr);
    }
};
