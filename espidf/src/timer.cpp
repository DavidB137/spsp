/**
 * @file timer.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Timer for SPSP purposes
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <thread>

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "spsp_timer.hpp"

namespace SPSP
{
    void _timer_callback(TimerHandle_t timer)
    {
        Timer* ctx = static_cast<Timer*>(pvTimerGetTimerID(timer));
        ctx->callCb();
    }

    Timer::Timer(const std::chrono::milliseconds interval,
                 std::function<void()> cb, bool cbInNewThread)
        : m_cb{cb}, cbInNewThread{cbInNewThread}
    {
        m_timer = xTimerCreate("SPSP::Timer", pdMS_TO_TICKS(interval.count()),
                               pdTRUE, this, _timer_callback);

        // Timer can't be created
        if (m_timer == nullptr) {
            throw TimerError();
        }

        // Timer can't be started
        if (xTimerStart(static_cast<TimerHandle_t>(m_timer), 0) == pdFAIL) {
            throw TimerError();
        }
    }

    Timer::~Timer()
    {
        TimerHandle_t timer = static_cast<TimerHandle_t>(m_timer);
        xTimerStop(timer, 0);
        xTimerDelete(timer, 0);
    }

    void Timer::callCb()
    {
        if (cbInNewThread) {
            std::thread t(m_cb);
            t.detach();
        } else {
            m_cb();
        }
    }
}
