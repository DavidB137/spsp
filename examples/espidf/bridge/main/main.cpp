#include "spsp_bridge.hpp"
#include "spsp_espnow.hpp"
#include "spsp_mqtt.hpp"
#include "spsp_wifi.hpp"

extern "C" void app_main()
{
    /**
     * Config
     */

    // WiFi config
    SPSP::WiFiConfig wifiConfig = {};
    wifiConfig.ssid = "SSID";
    wifiConfig.password = "PASSWORD";

    // ESPNOW config
    SPSP::LocalLayers::ESPNOW::Config espnowConfig = {};
    espnowConfig.ssid = 0x00000000;
    espnowConfig.password = "12345678123456781234567812345678";

    // MQTT config
    SPSP::FarLayers::MQTT::ClientConfig mqttConfig = {};
    mqttConfig.connection.uri = "mqtt://example.com";
    mqttConfig.connection.qos = 1;


    /**
     * Init
     */

    // Initialize WiFi
    SPSP::WiFi::getInstance().init(wifiConfig);

    // Create layers
    static SPSP::LocalLayers::ESPNOW::Layer ll{espnowConfig};
    static SPSP::FarLayers::MQTT::Layer fl{mqttConfig};

    // Create bridge
    static SPSP::Nodes::Bridge spsp{&ll, &fl};
}
