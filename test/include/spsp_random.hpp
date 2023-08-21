/**
 * @file spsp_random.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Random generator for testing
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

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
            for (size_t i = 0; i < len; i++) {
                buf[i] = rand();
            }
        }
    };
} // namespace SPSP
