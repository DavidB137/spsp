#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <string>
#include <thread>

#include "spsp_mqtt.hpp"
#include "spsp_mqtt_adapter.hpp"

using namespace SPSP;
using namespace std::chrono_literals;

const std::string SRC = "549b3d00da16ca2d";
const std::string TOPIC = "abc";
const std::string PAYLOAD = "123";
const FarLayers::MQTT::Config CONF = {
    .connection = {
        .timeout = 100ms,
    },
};
const std::string TOPIC_PUBLISH = CONF.pubTopicPrefix + "/" + SRC + "/" + TOPIC;

TEST_CASE("Check testing adapter properties", "[MQTT]") {
    FarLayers::MQTT::Adapter adapter{};

    CHECK(adapter.publish("", ""));
    CHECK(adapter.subscribe(""));
    CHECK(adapter.unsubscribe(""));
}

TEST_CASE("Check methods on not-failing adapter", "[MQTT]") {
    class Adapter : public FarLayers::MQTT::Adapter
    {
        bool publish(const std::string& topic, const std::string& payload)
        {
            CHECK(topic == TOPIC_PUBLISH);
            CHECK(payload == PAYLOAD);
            return true;
        }

        bool subscribe(const std::string& topic)
        {
            CHECK(topic == TOPIC);
            return true;
        }

        bool unsubscribe(const std::string& topic)
        {
            CHECK(topic == TOPIC);
            return true;
        }
    };

    // Instantiace
    Adapter adapter{};
    FarLayers::MQTT::MQTT mqtt{adapter, CONF};

    SECTION("Publish") {
        CHECK(mqtt.publish(SRC, TOPIC, PAYLOAD));
    }

    SECTION("Subscribe") {
        CHECK(mqtt.subscribe(TOPIC));
    }

    SECTION("Unsubscribe") {
        CHECK(mqtt.unsubscribe(TOPIC));
    }
}

TEST_CASE("Check methods on failing adapter", "[MQTT]") {
    class Adapter : public FarLayers::MQTT::Adapter
    {
        bool publish(const std::string& topic, const std::string& payload)
        {
            CHECK(topic == TOPIC_PUBLISH);
            CHECK(payload == PAYLOAD);
            return false;
        }

        bool subscribe(const std::string& topic)
        {
            CHECK(topic == TOPIC);
            return false;
        }

        bool unsubscribe(const std::string& topic)
        {
            CHECK(topic == TOPIC);
            return false;
        }
    };

    // Instantiace
    Adapter adapter{};
    FarLayers::MQTT::MQTT mqtt{adapter, CONF};

    SECTION("Publish") {
        CHECK(!mqtt.publish(SRC, TOPIC, PAYLOAD));
    }

    SECTION("Subscribe") {
        CHECK(!mqtt.subscribe(TOPIC));
    }

    SECTION("Unsubscribe") {
        CHECK(!mqtt.unsubscribe(TOPIC));
    }
}

TEST_CASE("Simulate connection failure", "[MQTT]") {
    class Adapter : public FarLayers::MQTT::Adapter
    {
        void setConnectedCb(FarLayers::MQTT::AdapterConnectedCb cb) {}
    };

    // Instantiace
    Adapter adapter{};
    REQUIRE_THROWS_AS(
        (FarLayers::MQTT::MQTT{adapter, CONF}),
        FarLayers::MQTT::ConnectionError
    );
}

TEST_CASE("Simulate reconnections", "[MQTT]") {
    class Adapter : public FarLayers::MQTT::Adapter
    {
        void setConnectedCb(FarLayers::MQTT::AdapterConnectedCb cb)
        {
            // Spawn new thread and leave it running independently
            std::thread t([cb]() {
                std::this_thread::sleep_for(10ms);
                cb();
                std::this_thread::sleep_for(10ms);
                cb();
                std::this_thread::sleep_for(10ms);
                cb();
            });
            t.detach();
        }
    };

    // Instantiace
    Adapter adapter{};
    REQUIRE_NOTHROW(FarLayers::MQTT::MQTT{adapter, CONF});

    // Wait for callbacks to finish
    std::this_thread::sleep_for(50ms);
}
