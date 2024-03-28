/**
 * @file logger.hpp
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

#define _SPSP_LOG(level, levelLogStrPre, levelLogStrPost, fmt, ...)          \
    do {                                                                     \
        if (SPSP::logLevel <= SPSP::LogLevel::level) {                       \
            fprintf(stderr, levelLogStrPre " %s: " fmt "\n" levelLogStrPost, \
                    SPSP_LOG_TAG, ##__VA_ARGS__);                            \
        }                                                                    \
    } while (0)

#if SPSP_LOG_NO_COLORS
    #define SPSP_LOGD(fmt, ...) _SPSP_LOG(DEBUG, "[D]", "", fmt, ##__VA_ARGS__)
    #define SPSP_LOGI(fmt, ...) _SPSP_LOG(INFO,  "[I]", "", fmt, ##__VA_ARGS__)
    #define SPSP_LOGW(fmt, ...) _SPSP_LOG(WARN,  "[W]", "", fmt, ##__VA_ARGS__)
    #define SPSP_LOGE(fmt, ...) _SPSP_LOG(ERROR, "[E]", "", fmt, ##__VA_ARGS__)
#else
    #define SPSP_LOGD(fmt, ...) _SPSP_LOG(DEBUG, "\033[0;34m[D]", "\033[0m", fmt, ##__VA_ARGS__)
    #define SPSP_LOGI(fmt, ...) _SPSP_LOG(INFO,  "\033[0;36m[I]", "\033[0m", fmt, ##__VA_ARGS__)
    #define SPSP_LOGW(fmt, ...) _SPSP_LOG(WARN,  "\033[0;33m[W]", "\033[0m", fmt, ##__VA_ARGS__)
    #define SPSP_LOGE(fmt, ...) _SPSP_LOG(ERROR, "\033[0;31m[E]", "\033[0m", fmt, ##__VA_ARGS__)
#endif
