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
    };
} // namespace SPSP
