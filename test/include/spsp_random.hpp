/**
 * @file spsp_random.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Random generator for testing
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <cstdint>
#include <cstdlib>

#include "spsp_random_if.hpp"

namespace SPSP
{
    /**
     * @brief Testing random generator
     *
     */
    class Random : public IRandom
    {
    public:
        inline void bytes(void* buf, size_t len) const noexcept
        {
            uint8_t* buf8 = static_cast<uint8_t*>(buf);

            for (size_t i = 0; i < len; i++) {
                buf8[i] = rand();
            }
        }
    };
} // namespace SPSP
