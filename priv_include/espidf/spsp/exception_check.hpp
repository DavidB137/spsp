/**
 * @file exception_check.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief ESP_ERROR_CHECK wrapper for exceptions
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include "esp_err.h"

#include "spsp/logger.hpp"

#define SPSP_ERROR_CHECK(x, exc) do {                                          \
        esp_err_t err = (x);                                                   \
        if (err != ESP_OK) {                                                   \
            SPSP_LOGE("Exception: expression '%s' in file '%s', line %d, "     \
                      "function '%s' returned 0x%x (%s)",                      \
                      #x, __FILE__, __LINE__, __ASSERT_FUNC, err,              \
                      esp_err_to_name(err));                                   \
            throw exc;                                                         \
        }                                                                      \
    } while(0)
