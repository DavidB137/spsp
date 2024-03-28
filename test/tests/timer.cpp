#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <future>

#include "spsp/timer.hpp"

using namespace SPSP;
using namespace std::chrono_literals;

TEST_CASE("10 timer ticks", "[Timer]") {
    std::promise<void> promise;

    // Store start timestamp
    auto startTS = std::chrono::system_clock::now();

    // Instantiate timer
    Timer timer(10ms, [&promise]() {
        promise.set_value();
    });

    for (size_t i = 1; i <= 10; i++) {
        // Wait for first tick
        promise.get_future().wait();

        // Reset promise
        promise = std::promise<void>();

        // Check first duration
        auto duration = std::chrono::system_clock::now() - startTS;
        CHECK(duration > 9ms*i);
        CHECK(duration < 11ms*i);
    }
}
