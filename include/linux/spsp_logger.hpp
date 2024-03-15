/**
 * @file spsp_logger.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Logger for SPSP
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <cstdint>
#include <cstdio>

namespace SPSP
{
    enum class LogLevel : uint_fast8_t
    {
        DEBUG = 0,
        INFO  = 1,
        WARN  = 2,
        ERROR = 3,
        OFF   = 255,
    };

    //! Global log level
    extern LogLevel logLevel;
} // namespace SPSP

#define _SPSP_LOG(level, levelLogStr, fmt, ...) do { \
        if (SPSP::logLevel <= SPSP::LogLevel::level) { \
            fprintf(stderr, levelLogStr " %s: " fmt "\n\033[0m", SPSP_LOG_TAG, ##__VA_ARGS__); \
        } \
    } while (0)

#define SPSP_LOGD(fmt, ...) _SPSP_LOG(DEBUG, "\033[0;34m[D]", fmt, ##__VA_ARGS__)
#define SPSP_LOGI(fmt, ...) _SPSP_LOG(INFO, "\033[0;36m[I]", fmt, ##__VA_ARGS__)
#define SPSP_LOGW(fmt, ...) _SPSP_LOG(WARN, "\033[0;33m[W]", fmt, ##__VA_ARGS__)
#define SPSP_LOGE(fmt, ...) _SPSP_LOG(ERROR, "\033[0;31m[E]", fmt, ##__VA_ARGS__)
