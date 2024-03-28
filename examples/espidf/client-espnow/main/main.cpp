#include <chrono>

#include "spsp/spsp.hpp"

using namespace std::chrono_literals;

extern "C" void app_main()
{
    /**
     * Config
     */

    // ESPNOW config
    SPSP::LocalLayers::ESPNOW::Config espnowConfig = {};
    espnowConfig.ssid = 0x00000000;
    espnowConfig.password = "12345678123456781234567812345678";


    /**
     * Init
     */

    // Initialize WiFi
    static SPSP::WiFi::Station wifi{{}};  // empty SSID -> doesn't connect to AP

    // Create local layer
    static SPSP::LocalLayers::ESPNOW::Adapter llAdapter{};
    static SPSP::LocalLayers::ESPNOW::ESPNOW ll{llAdapter, wifi, espnowConfig};

    // Create bridge
    static SPSP::Nodes::Client spsp{&ll};

    // Attempt to connect to the bridge
    while (true) {
        if (ll.connectToBridge()) {
            // Successfully connected
            break;
        }

        // Delay reconnection
        std::this_thread::sleep_for(5s);
    }


    /**
     * Publish data
     * Gets sent to the bridge.
     */

    bool delivered = spsp.publish("topic", "payload");

    if (delivered) {
        // ...
    } else {
        // ...
    }

    /**
     * Subscribe to topic
     * Gets sent to the bridge.
     */

    delivered = spsp.subscribe(
        "topic2",
        [](const std::string& topic, const std::string& payload) {
            // Do something when data on `topic2` are received
            // ...
        }
    );

    // Simulate work on something else
    std::this_thread::sleep_for(10s);

    // We don't need this subscription anymore
    delivered = spsp.unsubscribe("topic2");
}
