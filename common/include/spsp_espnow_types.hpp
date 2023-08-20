/**
 * @file spsp_espnow_types.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Types for ESPNOW classes
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include "spsp_local_addr_mac.hpp"
#include "spsp_local_message.hpp"

namespace SPSP::LocalLayers::ESPNOW
{
    using LocalAddrT = typename SPSP::LocalAddrMAC;
    using LocalMessageT = typename SPSP::LocalMessage<SPSP::LocalAddrMAC>;
} // namespace SPSP::LocalLayers::ESPNOW
