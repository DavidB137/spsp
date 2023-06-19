/**
 * @file spsp_local_message.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Local message classes
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <string>

#include "spsp_local_addr.hpp"

namespace SPSP
{
    /**
     * @brief Local message types
     * 
     */
    enum class LocalMessageType : uint8_t
    {
        NONE       = 0,
        OK         = 1,  // currently unused
        FAIL       = 2,  // currently unused
        PING       = 10,
        PONG       = 11,
        PUB        = 20,
        SUB_REQ    = 30,
        SUB_DATA   = 31,
    };

    /**
     * @brief Helper to convert `LocalMessageType` to string representation.
     * 
     * @param mt Message type
     * @return String representation
     */
    constexpr const char* localMessageTypeToStr(LocalMessageType mt) noexcept
    {
        switch (mt)
        {
            case LocalMessageType::NONE: return "NONE";
            case LocalMessageType::OK: return "OK";
            case LocalMessageType::FAIL: return "FAIL";
            case LocalMessageType::PING: return "PING";
            case LocalMessageType::PONG: return "PONG";
            case LocalMessageType::PUB: return "PUB";
            case LocalMessageType::SUB_REQ: return "SUB_REQ";
            case LocalMessageType::SUB_DATA: return "SUB_DATA";
            default: return "???";
        }
    }

    /**
     * @brief Local message representation
     * 
     * Used primarily for communication between `LocalLayer` and `Node` classes.
     * 
     */
    struct LocalMessage
    {
        LocalMessageType type;     //!< Type of message
        LocalAddr src = {};        //!< Source address
        std::string topic = "";    //!< Topic of message
        std::string payload = "";  //!< Payload of message

        /**
         * @brief Converts `LocalMessage` to printable string
         * 
         * Primarily for logging purposes
         * 
         * @return String representation of contained data
         */
        std::string toString() const
        {
            return std::string{localMessageTypeToStr(type)} + " " +
                (src.str.length() > 0 ? src.str : ".") + " " +
                (topic.length() > 0   ? topic   : "-") + " " +
                (payload.length() > 0 ? payload : "-");
        }
    };
} // namespace SPSP
