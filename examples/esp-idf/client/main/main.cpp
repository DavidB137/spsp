#include "spsp_client.hpp"
#include "spsp_espnow.hpp"
#include "spsp_wifi.hpp"

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
    SPSP::WiFi::getInstance().init();

    // Create local layer
    static SPSP::LocalLayers::ESPNOW::Layer ll{espnowConfig};

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
}
