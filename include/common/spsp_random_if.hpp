/**
 * @file spsp_random_if.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Platform-dependent random generator interface
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <cstdlib>

#include "spsp_exception.hpp"

namespace SPSP
{
    /**
     * @brief Random generator error
     *
     */
    class RandomGeneratorError : public SPSP::Exception
    {
        using SPSP::Exception::Exception;
    };

    /**
     * @brief Random generator interface
     *
     */
    class IRandom
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
         * @throw RandomGeneratorError Random generator error if generation fails.
         */
        virtual void bytes(void* buf, size_t len) const = 0;
    };
} // namespace SPSP
