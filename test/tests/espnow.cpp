#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <string>
#include <thread>

#include "spsp_espnow.hpp"
#include "spsp_espnow_adapter.hpp"
#include "spsp_nodes_dummy.hpp"
#include "spsp_wifi_dummy.hpp"
#include "spsp_chacha20.hpp"

using namespace SPSP;
using namespace std::chrono_literals;

using LocalAddrT = SPSP::LocalLayers::ESPNOW::LocalAddrT;
using LocalMessageT = SPSP::LocalLayers::ESPNOW::LocalMessageT;

const uint8_t ADDR_PEER_MAC[6] = {0x12, 0x23, 0x34, 0x45, 0x56, 0x01};
const uint8_t ADDR_PEER_MAC2[6] = {0x12, 0x23, 0x34, 0x45, 0x56, 0x02};
const LocalAddrT ADDR_PEER = ADDR_PEER_MAC;
const LocalAddrT ADDR_PEER2 = ADDR_PEER_MAC2;
const std::string TOPIC = "abc";
const std::string PAYLOAD = "123";
const LocalLayers::ESPNOW::Config CONF = {
    .ssid = 0x01020304,
    .password = std::string(32, 0x48),
    .connectToBridgeChannelWaiting = 50ms
};

const LocalMessageT MSG_BASE = {
    .type = LocalMessageType::PUB,
    .addr = ADDR_PEER,
    .topic = TOPIC,
    .payload = PAYLOAD
};

const LocalMessageT MSG_BASE_PROBE_RES = {
    .type = LocalMessageType::PROBE_RES,
    .addr = ADDR_PEER,
};

const LocalMessageT MSG_BASE_PROBE_RES2 = {
    .type = LocalMessageType::PROBE_RES,
    .addr = ADDR_PEER2,
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

// ESPNOW with support for direct receive of message
class ESPNOWRcvDir : public LocalLayers::ESPNOW::ESPNOW
{
public:
    using LocalLayers::ESPNOW::ESPNOW::ESPNOW;

    void receiveDirect(const LocalMessageT& msg, int rssi)
    {
        this->receive(msg, rssi);
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

TEST_CASE("Connect to bridge success", "[ESPNOW]") {
    WiFi::Dummy wifi{};
    AdapterSendSuccess adapter{};
    ESPNOWRcvDir espnow{adapter, wifi, CONF};

    const auto channelRestrictions = wifi.getChannelRestrictions();
    LocalLayers::ESPNOW::BridgeConnInfoRTC brInfo = {};

    SECTION("One response on last channel") {
        const auto probeResWaitTime = CONF.connectToBridgeChannelWaiting * (channelRestrictions.high - channelRestrictions.low) + 10ms;

        // Receive message simulator
        std::thread t1([&espnow, &probeResWaitTime]() {
            std::this_thread::sleep_for(probeResWaitTime);
            espnow.receiveDirect(MSG_BASE_PROBE_RES, -50);
        });

        std::thread t2([&espnow, &brInfo]() {
            REQUIRE(espnow.connectToBridge(nullptr, &brInfo));
        });

        t1.join();
        t2.join();

        CHECK(brInfo.ch == channelRestrictions.high);
    }

    SECTION("One response on first channel") {
        // Receive message simulator
        std::thread t1([&espnow]() {
            std::this_thread::sleep_for(10ms);
            espnow.receiveDirect(MSG_BASE_PROBE_RES, -50);
        });

        std::thread t2([&espnow, &brInfo]() {
            REQUIRE(espnow.connectToBridge(nullptr, &brInfo));
        });

        t1.join();
        t2.join();

        CHECK(brInfo.ch == channelRestrictions.low);
    }

    SECTION("One response on middle channel") {
        // Receive message simulator
        std::thread t1([&espnow]() {
            std::this_thread::sleep_for(CONF.connectToBridgeChannelWaiting + 10ms);
            espnow.receiveDirect(MSG_BASE_PROBE_RES, -50);
        });

        std::thread t2([&espnow, &brInfo]() {
            REQUIRE(espnow.connectToBridge(nullptr, &brInfo));
        });

        t1.join();
        t2.join();

        CHECK(brInfo.ch == channelRestrictions.low + 1);
    }

    SECTION("Three responses from two bridges on same channel") {
        // Receive message simulator
        std::thread t1([&espnow]() {
            std::this_thread::sleep_for(10ms);
            espnow.receiveDirect(MSG_BASE_PROBE_RES, -50);
            std::this_thread::sleep_for(10ms);
            espnow.receiveDirect(MSG_BASE_PROBE_RES2, -48);
            std::this_thread::sleep_for(10ms);
            espnow.receiveDirect(MSG_BASE_PROBE_RES, -45);
        });

        std::thread t2([&espnow, &brInfo]() {
            REQUIRE(espnow.connectToBridge(nullptr, &brInfo));
        });

        t1.join();
        t2.join();

        CHECK(brInfo.ch == channelRestrictions.low);
    }

    SECTION("Two responses with same RSSI") {
        // Receive message simulator
        std::thread t1([&espnow]() {
            std::this_thread::sleep_for(10ms);
            espnow.receiveDirect(MSG_BASE_PROBE_RES, -30);
            std::this_thread::sleep_for(CONF.connectToBridgeChannelWaiting);
            espnow.receiveDirect(MSG_BASE_PROBE_RES2, -40);
        });

        std::thread t2([&espnow, &brInfo]() {
            REQUIRE(espnow.connectToBridge(nullptr, &brInfo));
        });

        t1.join();
        t2.join();

        CHECK(brInfo.ch == channelRestrictions.low);
    }

    LocalAddrT brAddr = brInfo.addr;

    // Check correct bridge address and channel
    CHECK(static_cast<LocalAddr>(brAddr) == static_cast<LocalAddr>(ADDR_PEER));
}

TEST_CASE("Connect to bridge success - reconnect to bridge", "[ESPNOW]") {
    WiFi::Dummy wifi{};
    AdapterSendFail adapter{};
    LocalLayers::ESPNOW::ESPNOW espnow{adapter, wifi, CONF};

    // Populate retained bridge info
    LocalLayers::ESPNOW::BridgeConnInfoRTC brInfo = {};
    LocalLayers::ESPNOW::BridgeConnInfoRTC brInfoNew = {};
    ADDR_PEER.toMAC(brInfo.addr);
    brInfo.ch = 1;

    // This should return immediately
    REQUIRE(espnow.connectToBridge(&brInfo, &brInfoNew));

    // Check new retained bridge info
    LocalAddrT brAddr = brInfoNew.addr;
    CHECK(static_cast<LocalAddr>(brAddr) == static_cast<LocalAddr>(ADDR_PEER));
    CHECK(brInfoNew.ch == 1);
}
