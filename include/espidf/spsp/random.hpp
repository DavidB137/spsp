/**
 * @file random.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief ESP-IDF random generator
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <cstdlib>

#include "esp_random.h"

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
         * Platform dependent implementation, but should be cryptographically
         * secure.
         *
         * @param buf Buffer
         * @param len Length
         */
        inline void bytes(void* buf, size_t len) const noexcept
        {
            esp_fill_random(buf, len);
        }
    };
} // namespace SPSP
