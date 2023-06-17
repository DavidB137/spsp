/**
 * @file logger.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Logger for SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "esp_log.h"

#include "logger.hpp"

namespace SPSP
{
    template<typename... Targs> void Logger::error(Targs... args) const noexcept { ESP_LOGE(args...); }
    template<typename... Targs> void Logger::warn(Targs... args) const noexcept  { ESP_LOGW(args...); }
    template<typename... Targs> void Logger::info(Targs... args) const noexcept  { ESP_LOGI(args...); }
    template<typename... Targs> void Logger::debug(Targs... args) const noexcept { ESP_LOGD(args...); }
} // namespace SPSP
