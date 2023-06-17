/**
 * @file message.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Message classes
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <string>

namespace SPSP
{
    /**
     * @brief Message types
     * 
     */
    enum class MessageType : uint8_t
    {
        NONE       = 0,
        OK         = 1,  // currently unused
        FAIL       = 2,  // currently unused
        PING       = 10,
        PONG       = 11,
        PUB        = 20,
        SUB_REQ    = 30,
        SUB_RES    = 31,
    };

    /**
     * @brief Helper to convert `MessageType` to string representation.
     * 
     * @param mt Message type
     * @return String representation
     */
    constexpr const char* messageTypeToStr(MessageType mt) noexcept
    {
        switch (mt)
        {
            case MessageType::NONE: return "NONE";
            case MessageType::OK: return "OK";
            case MessageType::FAIL: return "FAIL";
            case MessageType::PING: return "PING";
            case MessageType::PONG: return "PONG";
            case MessageType::PUB: return "PUB";
            case MessageType::SUB_REQ: return "SUB_REQ";
            case MessageType::SUB_RES: return "SUB_RES";
            default: return "???";
        }
    }

    /**
     * @brief Message representation
     * 
     * Used primarily for communication between `LocalLayer` and `Node` classes.
     * 
     */
    struct Message
    {
        MessageType type;     //!< Type of message
        std::string src;      //!< Source address
        std::string topic;    //!< Topic of message
        std::string payload;  //!< Payload of message

        /**
         * @brief Converts `Message` to printable string
         * 
         * Primarily for logging purposes
         * 
         * @return String representation of contained data
         */
        std::string toString() const
        {
            return std::string{messageTypeToStr(type)}
                + " " + src
                + " " + topic
                + " " + payload;
        }
    };
} // namespace SPSP
