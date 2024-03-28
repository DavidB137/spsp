#include <catch2/catch_test_macros.hpp>

#include "spsp/espnow_ser_des.hpp"

using namespace SPSP;

using LocalAddrT = SPSP::LocalLayers::ESPNOW::LocalAddrT;
using LocalMessageT = SPSP::LocalLayers::ESPNOW::LocalMessageT;

const uint8_t ADDR_PEER_MAC[6] = {0x12, 0x23, 0x34, 0x45, 0x56, 0x01};
const LocalAddrT ADDR_PEER = ADDR_PEER_MAC;
const std::string TOPIC = "abc";
const std::string PAYLOAD = "123";
const LocalLayers::ESPNOW::Config CONF = {
    .ssid = 0x01020304,
    .password = std::string(32, 0x48)
};

const LocalMessageT MSG_BASE = {
    .type = LocalMessageType::PUB,
    .addr = ADDR_PEER,
    .topic = TOPIC,
    .payload = PAYLOAD
};

TEST_CASE("Deserialize garbage", "[ESPNOW]") {
    LocalLayers::ESPNOW::SerDes serdes(CONF);

    std::string garbage = "garbage";
    LocalMessageT deserialized;

    REQUIRE(!serdes.deserialize(ADDR_PEER, garbage, deserialized));
}

TEST_CASE("Serialize and deserialize", "[ESPNOW]") {
    LocalLayers::ESPNOW::SerDes serdes(CONF);

    // Serialize
    std::string serialized;
    serdes.serialize(MSG_BASE, serialized);

    // Deserialize
    LocalMessageT deserialized;

    SECTION("Same (unmodified) message") {
        REQUIRE(serdes.deserialize(ADDR_PEER, serialized, deserialized));
        CHECK(deserialized == MSG_BASE);
    }

    SECTION("Prepend serialized string") {
        serialized = " " + serialized;
        REQUIRE(!serdes.deserialize(ADDR_PEER, serialized, deserialized));
    }

    SECTION("Append to serialized string") {
        serialized += serialized + " ";
        REQUIRE(!serdes.deserialize(ADDR_PEER, serialized, deserialized));
    }

    SECTION("Shorten serialized string") {
        serialized.pop_back();
        REQUIRE(!serdes.deserialize(ADDR_PEER, serialized, deserialized));
    }

    SECTION("Simulate bitflip") {
        char orig = serialized[0];
        serialized[0] = orig + 1;
        REQUIRE(!serdes.deserialize(ADDR_PEER, serialized, deserialized));
    }
}
