/**
 * @file timer.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Timer for SPSP purposes
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <thread>

#include "spsp_timer.hpp"

namespace SPSP
{
    Timer::Timer(const std::chrono::milliseconds interval,
                 std::function<void()> cb)
        : m_interval{interval},
          m_nextExec{std::chrono::system_clock::now() + interval},
          m_run{true}, m_cb{cb}, m_thread{&Timer::handlerThread, this}
    {}

    Timer::~Timer()
    {
        {
            std::lock_guard lock{m_mutex};
            m_run = false;
        }

        // Notify handler thread
        m_cv.notify_one();

        // Wait for thread's return
        m_thread.join();
    }

    void Timer::handlerThread()
    {
        while (true) {
            // Call callback
            m_cb();

            m_nextExec += m_interval;

            // Wait for `m_interval` or destructor notification again
            std::unique_lock lock{m_mutex};
            if (m_cv.wait_until(lock, m_nextExec, [this]() { return !m_run; })) {
                // Destructor has been called
                break;
            }
        }
    }
}
