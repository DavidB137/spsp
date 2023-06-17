/**
 * @file espnow.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief ESP-NOW local layer for SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "logger.hpp"
#include "spsp_espnow.hpp"

namespace SPSP::LocalLayers::ESPNOW
{
    bool Layer::send(Message msg)
    {
        return true;
    }
} // namespace SPSP::LocalLayers::ESPNOW
