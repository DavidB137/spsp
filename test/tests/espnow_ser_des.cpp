#include <catch2/catch_test_macros.hpp>

#include "spsp_espnow_ser_des.hpp"

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

TEST_CASE("Serialize and deserialize same message", "[ESPNOW]") {
    LocalLayers::ESPNOW::SerDes serdes(CONF);

    // Serialize
    std::string serialized;
    serdes.serialize(MSG_BASE, serialized);

    // Deserialize
    LocalMessageT deserialized;
    REQUIRE(serdes.deserialize(ADDR_PEER, serialized, deserialized));

    CHECK(MSG_BASE.type == deserialized.type);
    CHECK(static_cast<LocalAddr>(MSG_BASE.addr) == static_cast<LocalAddr>(deserialized.addr));
    CHECK(MSG_BASE.topic == deserialized.topic);
    CHECK(MSG_BASE.payload == deserialized.payload);
}
