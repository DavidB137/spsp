/**
 * @file random.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief ESP-IDF random generator
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <cstdlib>
#include <sys/random.h>

#include "spsp/random_if.hpp"

namespace SPSP
{
    /**
     * @brief ESP random generator
     *
     * Truly random if WiFi or Bluetooth is enabled.
     */
    class Random : public IRandom
    {
    public:
        /**
         * @brief Generates `len` truly random bytes in `buf`
         *
         * @param buf Buffer
         * @param len Length
         * @throw RandomGeneratorError Random generator error if generation fails.
         */
        inline void bytes(void* buf, size_t len) const
        {
            if (getrandom(buf, len, 0) != len) {
                throw RandomGeneratorError("Generation failed");
            }
        }
    };
} // namespace SPSP
