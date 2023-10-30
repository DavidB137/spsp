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
    static SPSP::WiFi::Station wifi{{}};

    // Set WiFi channel
    //
    // As we use `LocalBroker` far layer, we don't need IP connectivity at all
    // and thus WiFi will be used purely by ESP-NOW to receive and send data
    // to clients.
    // We can set any WiFi channel we want (allowed by local law) and clients
    // will discover this bridge automatically.
    //
    // Remember, however, to use `wifi.setChannelRestrictions(...)` method on
    // both bridge and client, if you intend to use channels 12 or 13.
    wifi.setChannel(7);

    // Create layers
    static SPSP::LocalLayers::ESPNOW::Adapter llAdapter{};
    static SPSP::LocalLayers::ESPNOW::ESPNOW ll{llAdapter, wifi, espnowConfig};
    static SPSP::FarLayers::LocalBroker::LocalBroker fl{};

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
