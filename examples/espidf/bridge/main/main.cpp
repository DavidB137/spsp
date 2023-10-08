#include "spsp.hpp"

extern "C" void app_main()
{
    /**
     * Config
     */

    // WiFi config
    SPSP::WiFi::StationConfig wifiConfig = {};
    wifiConfig.ssid = "SSID";
    wifiConfig.password = "PASSWORD";

    // ESPNOW config
    SPSP::LocalLayers::ESPNOW::Config espnowConfig = {};
    espnowConfig.ssid = 0x00000000;
    espnowConfig.password = "12345678123456781234567812345678";

    // MQTT config
    SPSP::FarLayers::MQTT::Config mqttConfig = {};
    mqttConfig.connection.uri = "mqtt://example.com";
    mqttConfig.connection.qos = 1;


    /**
     * Init
     */

    // Initialize WiFi
    static SPSP::WiFi::Station wifi{wifiConfig};

    // Create layers
    static SPSP::LocalLayers::ESPNOW::Adapter llAdapter{};
    static SPSP::LocalLayers::ESPNOW::ESPNOW ll{llAdapter, wifi, espnowConfig};
    static SPSP::FarLayers::MQTT::Adapter flAdapter{mqttConfig};
    static SPSP::FarLayers::MQTT::MQTT fl{flAdapter, mqttConfig};

    // Create bridge
    static SPSP::Nodes::Bridge spsp{&ll, &fl};

    /**
     * Publish and subscribe
     * API is the same as for the client.
     * Typically, you publish and subscribe on clients only,
     * but you can do the same on bridges too.
     */

    bool delivered = spsp.publish("topic", "payload");

    if (delivered) {
        // ...
    } else {
        // ...
    }

    delivered = spsp.subscribe(
        "topic2",
        [](const std::string& topic, const std::string& payload) {
            // Do something when data on `topic2` are received
            // ...
        }
    );

    // We don't need this subscription anymore
    delivered = spsp.unsubscribe("topic2");
}
