/**
 * @file exception.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Base of all SPSP exceptions
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <exception>
#include <string>

namespace SPSP
{
    /**
     * @brief Base SPSP exception
     *
     */
    class Exception : public std::exception
    {
        std::string m_msg;
    
    public:
        /**
         * @brief Constructs a new exception
         *
         * @param msg Message
         */
        explicit Exception(const std::string& msg) : m_msg{msg} {}

        /**
         * @brief Returns exception's message
         *
         * @return Message
         */
        virtual const char* what() const noexcept
        {
            return m_msg.c_str();
        }
    };
} // namespace SPSP
