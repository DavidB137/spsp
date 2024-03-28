/**
 * @file local_message.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Local message classes
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <string>

#include "spsp/local_addr.hpp"

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
        PROBE_REQ  = 10,
        PROBE_RES  = 11,
        PUB        = 20,
        SUB_REQ    = 30,
        SUB_DATA   = 31,
        UNSUB      = 32,
        TIME_REQ   = 40,
        TIME_RES   = 41,
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
            case LocalMessageType::PROBE_REQ: return "PROBE_REQ";
            case LocalMessageType::PROBE_RES: return "PROBE_RES";
            case LocalMessageType::PUB: return "PUB";
            case LocalMessageType::SUB_REQ: return "SUB_REQ";
            case LocalMessageType::SUB_DATA: return "SUB_DATA";
            case LocalMessageType::UNSUB: return "UNSUB";
            case LocalMessageType::TIME_REQ: return "TIME_REQ";
            case LocalMessageType::TIME_RES: return "TIME_RES";
            default: return "???";
        }
    }

    /**
     * @brief Local message representation
     *
     * Used primarily for communication between `LocalLayer` and `Node` classes.
     *
     * @tparam TLocalAddr Type of local address
     */
    template <typename TLocalAddr>
    struct LocalMessage
    {
        using LocalAddrT = TLocalAddr;

        LocalMessageType type;     //!< Type of message
        TLocalAddr addr = {};      //!< Source/destination address
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
                (addr.str.length() > 0 ? addr.str : "(no addr)") + " " +
                (topic.length() > 0    ? topic   : "(no topic)") + " " +
                "(" + std::to_string(payload.length()) + " B payload)";
        }

        bool operator==(const LocalMessage& other) const
        {
            return type == other.type
                && addr == other.addr
                && topic == other.topic
                && payload == other.payload;
        }
    };
} // namespace SPSP

// Define hasher function
template <typename TLocalAddr>
struct std::hash<SPSP::LocalMessage<TLocalAddr>>
{
    std::size_t operator()(SPSP::LocalMessage<TLocalAddr> const& msg) const noexcept
    {
        return std::hash<SPSP::LocalMessageType>{}(msg.type)
             + std::hash<TLocalAddr>{}(msg.addr)
             + std::hash<std::string>{}(msg.topic)
             + std::hash<std::string>{}(msg.payload);
    }
};
