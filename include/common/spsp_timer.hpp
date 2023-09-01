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
#include <condition_variable>
#include <exception>
#include <functional>
#include <mutex>
#include <thread>

namespace SPSP
{
    /**
     * @brief Simple timer for SPSP purposes
     *
     * (... but you can also use it outside of SPSP.)
     *
     */
    class Timer
    {
        std::mutex m_mutex;                                //!< Mutex for conditional variable
        std::chrono::milliseconds m_interval;              //!< Timer interval
        std::chrono::system_clock::time_point m_nextExec;  //!< Next execution time point
        bool m_run;                                        //!< Whether to continue running
        std::function<void()> m_cb;                        //!< Callback
        std::thread m_thread;                              //!< Handler thread
        std::condition_variable m_cv;                      //!< Conditional variable (to sync destruction of handler thread)

    public:
        /**
         * @brief Constructs a new timer
         *
         * First execution of `cb` will be after first `interval` expires
         * (not immediately).
         *
         * @param interval Interval of timer
         * @param cb Callback (`std::bind` can be used)
         * @param cbInNewThread Whether to call callback in new thread (prevents stack overflows)
         */
        Timer(const std::chrono::milliseconds interval,
              std::function<void()> cb);

        /**
         * @brief Destroys the timer
         *
         */
        ~Timer();

    protected:
        /**
         * @brief Timer handler thread
         *
         */
        void handlerThread();
    };
} // namespace SPSP
