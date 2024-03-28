/**
 * @file local_addr_mac.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Local layer address container for MAC address
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <string>
#include <vector>

#include "spsp/local_addr.hpp"

namespace SPSP
{
    /**
     * @brief Local layer address container for MAC address
     *
     * MAC as addess type is very common, so implement everything here.
     */
    struct LocalAddrMAC : public LocalAddr
    {
        /**
         * @brief Constructs a new object
         *
         * @param mac MAC address (00:00:00:00:00:00 is assumed if `nullptr`)
         */
        LocalAddrMAC(const uint8_t* mac = nullptr);

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
