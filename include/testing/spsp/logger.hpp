/**
 * @file logger.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Logger for testing
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <cstdio>

#define SPSP_LOGE(fmt, ...) fprintf(stderr, "\033[0;31m[E] %s: " fmt "\n\033[0m", SPSP_LOG_TAG, ##__VA_ARGS__)
#define SPSP_LOGW(fmt, ...) fprintf(stderr, "\033[0;33m[W] %s: " fmt "\n\033[0m", SPSP_LOG_TAG, ##__VA_ARGS__)
#define SPSP_LOGI(fmt, ...) fprintf(stderr, "\033[0;36m[I] %s: " fmt "\n\033[0m", SPSP_LOG_TAG, ##__VA_ARGS__)
#define SPSP_LOGD(fmt, ...) fprintf(stderr, "\033[0;34m[D] %s: " fmt "\n\033[0m", SPSP_LOG_TAG, ##__VA_ARGS__)
