/**
 * @file timer.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Testing timer
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <thread>

#include "spsp_timer.hpp"

namespace SPSP
{
    Timer::Timer(const std::chrono::milliseconds interval,
                 std::function<void()> cb, bool cbInNewThread)
        : m_cb{cb}, cbInNewThread{cbInNewThread}
    {
    }

    Timer::~Timer()
    {
    }

    void Timer::callCb()
    {
    }
}
