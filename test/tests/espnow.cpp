#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <string>
#include <thread>
#include <iostream>

#include "spsp_espnow.hpp"
#include "spsp_espnow_adapter.hpp"
#include "spsp_nodes_dummy.hpp"
#include "spsp_wifi_dummy.hpp"
#include "spsp_chacha20.hpp"

using namespace SPSP;
using namespace std::chrono_literals;

using LocalAddrT = SPSP::LocalLayers::ESPNOW::LocalAddrT;
using LocalMessageT = SPSP::LocalLayers::ESPNOW::LocalMessageT;

const uint8_t ADDR_LOCAL_MAC[6] = {0x12, 0x23, 0x34, 0x45, 0x56, 0x00};
const uint8_t ADDR_PEER_MAC[6] = {0x12, 0x23, 0x34, 0x45, 0x56, 0x01};
const LocalAddrT ADDR_LOCAL = ADDR_LOCAL_MAC;
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

class AdapterSendSuccess : public LocalLayers::ESPNOW::Adapter
{
    void send(const LocalAddrT& dst, const std::string& data) const
    {
        std::thread t(this->getSendCb(), dst, true);
        t.detach();
    }
};

class AdapterSendSuccessWait : public LocalLayers::ESPNOW::Adapter
{
    void send(const LocalAddrT& dst, const std::string& data) const
    {
        std::thread t([this, &dst]() {
            std::this_thread::sleep_for(10ms);
            this->getSendCb()(dst, true);
        });
        t.detach();
    }
};

class AdapterSendFail : public LocalLayers::ESPNOW::Adapter
{
    void send(const LocalAddrT& dst, const std::string& data) const
    {
        std::thread t(this->getSendCb(), dst, false);
        t.detach();
    }
};

TEST_CASE("Send adapter success", "[ESPNOW]") {
    WiFi::Dummy wifi{};
    AdapterSendSuccess adapter{};
    LocalLayers::ESPNOW::ESPNOW espnow{adapter, wifi, CONF};

    auto msg = MSG_BASE;

    SECTION("Basic") {
        REQUIRE(espnow.send(msg));
    }

    SECTION("Too long payload") {
        msg.payload = std::string(250, '0');
        REQUIRE(!espnow.send(msg));
    }

    SECTION("Empty address without bridge connected") {
        msg.addr = LocalAddrT{};
        REQUIRE(!espnow.send(msg));
    }
}

TEST_CASE("Send adapter fail", "[ESPNOW]") {
    WiFi::Dummy wifi{};
    AdapterSendFail adapter{};
    LocalLayers::ESPNOW::ESPNOW espnow{adapter, wifi, CONF};

    SECTION("Basic") {
        REQUIRE(!espnow.send(MSG_BASE));
    }
}

TEST_CASE("Send multiple messages to same address", "[ESPNOW]") {
    WiFi::Dummy wifi{};
    AdapterSendSuccessWait adapter{};
    LocalLayers::ESPNOW::ESPNOW espnow{adapter, wifi, CONF};

    auto sendLambda = [&espnow]() {
        REQUIRE(espnow.send(MSG_BASE));
    };

    std::thread t1(sendLambda), t2(sendLambda), t3(sendLambda);
    t1.join();
    t2.join();
    t3.join();
}

TEST_CASE("Check encrypted message", "[ESPNOW]") {
    class Adapter : public LocalLayers::ESPNOW::Adapter
    {
        void send(const LocalAddrT& dst, const std::string& data) const
        {
            // TODO: this may be incorrect, because nonce and value of
            // reserved bytes may change
            char correctData[] = {
                4, 3, 2, 1,                       // SSID
                37, 93, 5, 23, 88, -23, 94, -44,  // Nonce
                1,                                // Version
                -25,                              // Encrypted type
                -66, 17, 103,                     // Encrypted reserved
                63,                               // Encrypted checksum
                -104,                             // Encrypted topic length
                7,                                // Encrypted payload length
                -21, 87, -106,                    // Encrypted topic
                -87, -10, -92                     // Encrypted payload
            };

            REQUIRE(data.length() == 26);
            CHECK(data == std::string(correctData, 26));

            // Complete sending
            std::thread t(this->getSendCb(), dst, true);
            t.detach();
        }
    };

    WiFi::Dummy wifi{};
    Adapter adapter{};
    LocalLayers::ESPNOW::ESPNOW espnow{adapter, wifi, CONF};

    CHECK(espnow.send(MSG_BASE));
}

TEST_CASE("Send and receive the same message", "[ESPNOW]") {
    class LocalNode : public Nodes::DummyLocalNode<LocalLayers::ESPNOW::ESPNOW>
    {
        using Nodes::DummyLocalNode<LocalLayers::ESPNOW::ESPNOW>::DummyLocalNode;

        bool processPub(const LocalMessageT& req,
                        int rssi = NODE_RSSI_UNKNOWN)
        {
            // Check same message was received
            CHECK(req.type == MSG_BASE.type);
            CHECK(req.addr == MSG_BASE.addr);
            CHECK(req.topic == MSG_BASE.topic);
            CHECK(req.payload == MSG_BASE.payload);
            return true;
        }
    };

    class Adapter : public LocalLayers::ESPNOW::Adapter
    {
        void send(const LocalAddrT& dst, const std::string& data) const
        {
            // Confirm delivery
            std::thread t1(this->getSendCb(), dst, true);
            t1.detach();

            // Receive same data
            std::thread t2(this->getRecvCb(), dst, data, 0);
            t2.join();
        }
    };

    WiFi::Dummy wifi{};
    Adapter adapter{};
    LocalLayers::ESPNOW::ESPNOW espnow{adapter, wifi, CONF};
    LocalNode node(&espnow);

    CHECK(espnow.send(MSG_BASE));
}

TEST_CASE("Connect to bridge fail - no response", "[ESPNOW]") {
    WiFi::Dummy wifi{};
    AdapterSendSuccess adapter{};
    LocalLayers::ESPNOW::ESPNOW espnow{adapter, wifi, CONF};

    CHECK(!espnow.connectToBridge());
}

TEST_CASE("Connect to bridge fail - adapter fail", "[ESPNOW]") {
    WiFi::Dummy wifi{};
    AdapterSendFail adapter{};
    LocalLayers::ESPNOW::ESPNOW espnow{adapter, wifi, CONF};

    CHECK(!espnow.connectToBridge());
}

TEST_CASE("Connect to bridge fail - connection info doesn't change", "[ESPNOW]") {
    WiFi::Dummy wifi{};
    AdapterSendFail adapter{};
    LocalLayers::ESPNOW::ESPNOW espnow{adapter, wifi, CONF};
    LocalLayers::ESPNOW::BridgeConnInfoRTC brInfo = {};

    REQUIRE(brInfo.ch == 0);
    CHECK(!espnow.connectToBridge(nullptr, &brInfo));
    CHECK(brInfo.ch == 0);
}

// TODO: more connect to bridge tests
