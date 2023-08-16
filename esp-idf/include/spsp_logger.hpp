/**
 * @file spsp_logger.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Logger for SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "esp_log.h"

#pragma once

#define SPSP_LOGE(fmt, ...) ESP_LOGE(SPSP_LOG_TAG, fmt, __VA_ARGS__)
#define SPSP_LOGW(fmt, ...) ESP_LOGW(SPSP_LOG_TAG, fmt, __VA_ARGS__)
#define SPSP_LOGI(fmt, ...) ESP_LOGI(SPSP_LOG_TAG, fmt, __VA_ARGS__)
#define SPSP_LOGD(fmt, ...) ESP_LOGD(SPSP_LOG_TAG, fmt, __VA_ARGS__)
