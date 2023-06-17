/**
 * @file logger.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Logger for SPSP
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef SPSP_LOGGER_HPP
#define SPSP_LOGGER_HPP

#include <string>

namespace SPSP
{
    class Logger
    {
        std::string tag;  //!< Logging tag
    public:
        /**
         * @brief Constructs a new logger
         * 
         * @param moduleTag Optional module name appended to the tag
         */
        Logger(std::string moduleTag = "") : tag{"SPSP"}
        {
            // Append module tag
            if (moduleTag.length() > 0) {
                tag += "/" + moduleTag;
            }
        }

        // Logging functions
        template<typename... Targs> inline void error(Targs... args) const noexcept;
        template<typename... Targs> inline void warn(Targs... args) const noexcept;
        template<typename... Targs> inline void info(Targs... args) const noexcept;
        template<typename... Targs> inline void debug(Targs... args) const noexcept;
    };
} // namespace SPSP

#endif
