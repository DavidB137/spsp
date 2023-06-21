/**
 * @file spsp_local_addr.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Local layer address container
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <string>
#include <vector>

namespace SPSP
{
    /**
     * @brief Local layer address container
     * 
     * Internal representation is decisive. Two addresses are the same if they
     * have the same internal representation.
     * 
     * Printable string is also passed to far layer (MQTT).
     */
    struct LocalAddr
    {
        std::vector<uint8_t> addr;  //!< Internal address representation
        std::string str;            //!< Printable string (also used in MQTT topic)

        bool operator==(const LocalAddr &other) const
        {
            return addr == other.addr;
        }

        /**
         * @brief Checks whether the address is empty
         * 
         * @return true Local address is empty
         * @return false Local address is not empty
         */
        inline bool empty() const
        {
            return addr.empty();
        }
    };
}

// Define hasher function
template<>
struct std::hash<SPSP::LocalAddr>
{
    std::size_t operator()(SPSP::LocalAddr const& addr) const noexcept
    {
        // Convert internal representation to string
        std::string addrStr(addr.addr.begin(), addr.addr.end());

        // Hash it
        return std::hash<std::string>{}(addrStr);
    }
};
