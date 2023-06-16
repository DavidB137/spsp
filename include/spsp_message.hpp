/**
 * @file message.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Message classes
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef SPSP_MESSAGE_HPP
#define SPSP_MESSAGE_HPP

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
     * @brief Internal message representation
     * 
     * Used primarily for communication between `LocalLayer` and `Node` classes.
     * 
     */
    struct MessageInternal
    {
        MessageType type;     //!< Type of message
        std::string src;      //!< Source address
        std::string topic;    //!< Topic of message
        std::string payload;  //!< Payload of message
    };
} // namespace SPSP

#endif
