#include "spsp.hpp"

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


    /**
     * Do something
     */

    // Publish data
    // Gets sent to the bridge
    bool delivered = spsp.publish("topic", "payload");

    if (delivered) {
        // ...
    } else {
        // ...
    }

    // Also see: spsp.subscribe(), spsp.unsubscribe()
}
