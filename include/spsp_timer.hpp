/**
 * @file spsp_timer.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Timer for SPSP purposes
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <chrono>
#include <exception>
#include <functional>

namespace SPSP
{
    /**
     * @brief Timer error
     * 
     * Thrown when timer cannot be created or started.
     */
    class TimerError : public std::exception {};

    /**
     * @brief Simple timer for SPSP purposes
     * 
     * (... but you can also use it outside of SPSP.)
     * 
     * Basically it's just a plaform-dependent wrapper.
     * On ESP-based platforms wraps FreeRTOS timers.
     * 
     */
    class Timer
    {
    protected:
        void* m_timer;               //!< Timer handle (platform dependent)
        std::function<void()> m_cb;  //!< Callback

    public:
        /**
         * @brief Constructs a new timer
         * 
         * First execution of `cb` will be after first `interval` expires
         * (not immediately).
         * 
         * @param interval Interval of timer
         * @param cb Callback (`std::bind` can be used)
         */
        Timer(const std::chrono::milliseconds interval, std::function<void()> cb);

        /**
         * @brief Destroys the timer
         * 
         */
        ~Timer();

        /**
         * @brief Calls stored callback
         * 
         * A new thread is spawned for this call.
         */
        void callCb();
    };
} // namespace SPSP
