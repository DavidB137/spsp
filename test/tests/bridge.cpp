#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "spsp_bridge.hpp"
#include "spsp_layers_dummy.hpp"
#include "spsp_local_addr.hpp"
#include "spsp_local_addr_mac.hpp"
#include "spsp_local_message.hpp"
#include "spsp_version.hpp"

using namespace SPSP;
using namespace std::chrono_literals;

using PubsSetT = typename FarLayers::DummyFarLayer::PubsSetT;
using SubsSetT = typename FarLayers::DummyFarLayer::SubsSetT;
using SubsLogT = typename FarLayers::DummyFarLayer::SubsLogT;
using SentMsgsSetT = typename LocalLayers::DummyLocalLayer::SentMsgsSetT;

const LocalAddr ADDR_PEER1 = { .addr = {0, 0, 0, 1}, .str = "0001" };
const LocalAddr ADDR_PEER2 = { .addr = {0, 0, 0, 2}, .str = "0002" };
const std::string TOPIC = "abc";
const std::string TOPIC_SUFFIX = "abc/def";
const std::string TOPIC_SL_WILD = "abc/+";
const std::string TOPIC_ML_WILD = "abc/#";
const std::string PAYLOAD = "123";
const Nodes::BridgeConfig CONF = {
    .reporting = {
        .version = false,
        .probePayload = false,
        .rssiOnProbe = false,
        .rssiOnPub = false,
        .rssiOnSub = false,
        .rssiOnUnsub = false,
    },
    .subDB = {
        .interval = 10ms,
        .subLifetime = 100ms,
    },
};
const LocalMessage<LocalAddr> MSG_SUB1 = {
    .type = LocalMessageType::SUB_REQ,
    .addr = ADDR_PEER1,
    .topic = TOPIC,
    .payload = PAYLOAD
};
const LocalMessage<LocalAddr> MSG_SUB2 = {
    .type = LocalMessageType::SUB_REQ,
    .addr = ADDR_PEER2,
    .topic = TOPIC,
    .payload = PAYLOAD
};

TEST_CASE("Publish", "[Bridge]") {
    LocalLayers::DummyLocalLayer ll{};
    FarLayers::DummyFarLayer fl{};
    Nodes::Bridge br{&ll, &fl, CONF};

    CHECK(br.publish(TOPIC, PAYLOAD));
    CHECK(fl.getPubs() == PubsSetT{
        "PUB " + LocalAddrMAC::local().str + " " + TOPIC + " " + PAYLOAD
    });
}

TEST_CASE("Subscribe", "[Bridge]") {
    LocalLayers::DummyLocalLayer ll{};
    FarLayers::DummyFarLayer fl{};
    Nodes::Bridge br{&ll, &fl, CONF};

    SECTION("Single topic (and check infinite lifetime") {
        REQUIRE(br.subscribe(TOPIC, nullptr));

        std::this_thread::sleep_for(CONF.subDB.subLifetime
                                    + 2*CONF.subDB.interval);

        CHECK(fl.getSubs() == SubsSetT{TOPIC});
        CHECK(fl.getSubsLog() == SubsLogT{TOPIC});
    }

    SECTION("Two different topics") {
        REQUIRE(br.subscribe(TOPIC, nullptr));
        REQUIRE(br.subscribe(TOPIC_SUFFIX, nullptr));

        CHECK(fl.getSubs() == SubsSetT{TOPIC, TOPIC_SUFFIX});
        CHECK(fl.getSubsLog() == SubsLogT{TOPIC, TOPIC_SUFFIX});
    }

    SECTION("Twice to the same topic") {
        REQUIRE(br.subscribe(TOPIC, nullptr));
        REQUIRE(br.subscribe(TOPIC, nullptr));

        CHECK(fl.getSubs() == SubsSetT{TOPIC});
        CHECK(fl.getSubsLog() == SubsLogT{TOPIC});
    }

    SECTION("Single level wildcard topic") {
        REQUIRE(br.subscribe(TOPIC_SL_WILD, nullptr));

        CHECK(fl.getSubs() == SubsSetT{TOPIC_SL_WILD});
        CHECK(fl.getSubsLog() == SubsLogT{TOPIC_SL_WILD});
    }

    SECTION("Multi level wildcard topic") {
        REQUIRE(br.subscribe(TOPIC_ML_WILD, nullptr));

        CHECK(fl.getSubs() == SubsSetT{TOPIC_ML_WILD});
        CHECK(fl.getSubsLog() == SubsLogT{TOPIC_ML_WILD});
    }

    SECTION("Empty topic") {
        REQUIRE(!br.subscribe("", nullptr));

        CHECK(fl.getSubs() == SubsSetT{});
        CHECK(fl.getSubsLog() == SubsLogT{});
    }

    CHECK(fl.getUnsubsLog() == SubsLogT{});
}

TEST_CASE("Resubscribe", "[Bridge]") {
    LocalLayers::DummyLocalLayer ll{};
    FarLayers::DummyFarLayer fl{};
    Nodes::Bridge br{&ll, &fl, CONF};

    REQUIRE(br.subscribe(TOPIC, nullptr));
    REQUIRE(br.subscribe(TOPIC_SUFFIX, nullptr));
    REQUIRE(br.subscribe(TOPIC_SL_WILD, nullptr));

    br.resubscribeAll();

    CHECK(fl.getSubs() == SubsSetT{TOPIC, TOPIC_SUFFIX, TOPIC_SL_WILD});
    CHECK(fl.getSubsLog().size() == 6);
    CHECK(fl.getUnsubsLog() == SubsLogT{});
}

TEST_CASE("Unsubscribe", "[Bridge]") {
    LocalLayers::DummyLocalLayer ll{};
    FarLayers::DummyFarLayer fl{};
    Nodes::Bridge br{&ll, &fl, CONF};

    REQUIRE(br.subscribe(TOPIC, nullptr));
    REQUIRE(br.subscribe(TOPIC_SUFFIX, nullptr));
    REQUIRE(br.subscribe(TOPIC_SL_WILD, nullptr));
    REQUIRE(br.subscribe(TOPIC_ML_WILD, nullptr));

    CHECK(fl.getSubsLog() == SubsLogT{TOPIC, TOPIC_SUFFIX, TOPIC_SL_WILD,
                                     TOPIC_ML_WILD});

    SECTION("Simple topic") {
        REQUIRE(br.unsubscribe(TOPIC));

        CHECK(fl.getSubs() == SubsSetT{TOPIC_SUFFIX, TOPIC_SL_WILD,
                                      TOPIC_ML_WILD});
        CHECK(fl.getUnsubsLog() == SubsLogT{TOPIC});
    }

    SECTION("Topic and it's prefix") {
        REQUIRE(br.unsubscribe(TOPIC_SUFFIX));
        REQUIRE(br.unsubscribe(TOPIC));

        CHECK(fl.getSubs() == SubsSetT{TOPIC_SL_WILD, TOPIC_ML_WILD});
        CHECK(fl.getUnsubsLog() == SubsLogT{TOPIC_SUFFIX, TOPIC});
    }

    SECTION("Single level wildcard") {
        REQUIRE(br.unsubscribe(TOPIC_SL_WILD));

        CHECK(fl.getSubs() == SubsSetT{TOPIC, TOPIC_SUFFIX, TOPIC_ML_WILD});
        CHECK(fl.getUnsubsLog() == SubsLogT{TOPIC_SL_WILD});
    }

    SECTION("Multi level wildcard") {
        REQUIRE(br.unsubscribe(TOPIC_ML_WILD));

        CHECK(fl.getSubs() == SubsSetT{TOPIC, TOPIC_SUFFIX, TOPIC_SL_WILD});
        CHECK(fl.getUnsubsLog() == SubsLogT{TOPIC_ML_WILD});
    }

    SECTION("All subscribed topics in random order") {
        REQUIRE(br.unsubscribe(TOPIC_SUFFIX));
        REQUIRE(br.unsubscribe(TOPIC_ML_WILD));
        REQUIRE(br.unsubscribe(TOPIC));
        REQUIRE(br.unsubscribe(TOPIC_SL_WILD));

        CHECK(fl.getSubs() == SubsSetT{});
        CHECK(fl.getUnsubsLog() == SubsLogT{TOPIC_SUFFIX, TOPIC_ML_WILD, TOPIC,
                                           TOPIC_SL_WILD});
    }

    SECTION("Non-existing topic") {
        REQUIRE(!br.unsubscribe(TOPIC + "x"));

        CHECK(fl.getSubs() == SubsSetT{TOPIC, TOPIC_SUFFIX, TOPIC_SL_WILD,
                                      TOPIC_ML_WILD});
        CHECK(fl.getUnsubsLog() == SubsLogT{});
    }

    SECTION("Empty topic") {
        REQUIRE(!br.unsubscribe(""));

        CHECK(fl.getSubs() == SubsSetT{TOPIC, TOPIC_SUFFIX, TOPIC_SL_WILD,
                                      TOPIC_ML_WILD});
        CHECK(fl.getUnsubsLog() == SubsLogT{});
    }
}

TEST_CASE("Receive from local layer", "[Bridge]") {
    LocalLayers::DummyLocalLayer ll{};
    FarLayers::DummyFarLayer fl{};
    Nodes::Bridge br{&ll, &fl, CONF};

    auto msg = MSG_SUB1;

    SECTION("PROBE_REQ") {
        msg.type = LocalMessageType::PROBE_REQ;
        ll.receiveDirect(msg);

        std::this_thread::sleep_for(10ms);

        CHECK(ll.getSentMsgs() == SentMsgsSetT{{
            .type = LocalMessageType::PROBE_RES,
            .addr = msg.addr,
            .topic = msg.topic,  // Topic is preserved
            .payload = VERSION,
        }});
    }

    SECTION("PROBE_RES") {
        msg.type = LocalMessageType::PROBE_RES;
        ll.receiveDirect(msg);

        std::this_thread::sleep_for(10ms);

        CHECK(ll.getSentMsgs() == SentMsgsSetT{});
    }

    SECTION("PUB") {
        msg.type = LocalMessageType::PUB;
        ll.receiveDirect(msg);

        std::this_thread::sleep_for(10ms);

        CHECK(ll.getSentMsgs() == SentMsgsSetT{});
        CHECK(fl.getPubs() == PubsSetT{msg.toString()});
    }

    SECTION("SUB_REQ") {
        msg.type = LocalMessageType::SUB_REQ;
        ll.receiveDirect(msg);

        std::this_thread::sleep_for(10ms);

        CHECK(ll.getSentMsgs() == SentMsgsSetT{});
        CHECK(fl.getSubs() == SubsSetT{msg.topic});
        CHECK(fl.getSubsLog() == SubsLogT{msg.topic});
    }

    SECTION("SUB_DATA") {
        msg.type = LocalMessageType::SUB_DATA;
        ll.receiveDirect(msg);

        std::this_thread::sleep_for(10ms);

        CHECK(ll.getSentMsgs() == SentMsgsSetT{});
    }

    SECTION("UNSUB") {
        msg.type = LocalMessageType::UNSUB;
        ll.receiveDirect(msg);

        std::this_thread::sleep_for(10ms);

        CHECK(ll.getSentMsgs() == SentMsgsSetT{});
    }

    SECTION("UNSUB after SUB_REQ") {
        msg.type = LocalMessageType::SUB_REQ;
        ll.receiveDirect(msg);

        msg.type = LocalMessageType::UNSUB;
        ll.receiveDirect(msg);

        CHECK(ll.getSentMsgs() == SentMsgsSetT{});
        CHECK(fl.getSubs() == SubsSetT{});
        CHECK(fl.getSubsLog() == SubsLogT{msg.topic});
        CHECK(fl.getUnsubsLog() == SubsLogT{msg.topic});
    }

    SECTION("SUB_REQ expires") {
        msg.type = LocalMessageType::SUB_REQ;
        ll.receiveDirect(msg);

        std::this_thread::sleep_for(CONF.subDB.subLifetime
                                    + 2*CONF.subDB.interval);

        CHECK(ll.getSentMsgs() == SentMsgsSetT{});
        CHECK(fl.getSubs() == SubsSetT{});
        CHECK(fl.getSubsLog() == SubsLogT{msg.topic});
        CHECK(fl.getUnsubsLog() == SubsLogT{msg.topic});
    }

    SECTION("PUB with empty topic") {
        msg.type = LocalMessageType::PUB;
        msg.topic = "";
        ll.receiveDirect(msg);

        std::this_thread::sleep_for(10ms);

        CHECK(ll.getSentMsgs() == SentMsgsSetT{});
        CHECK(fl.getPubs() == PubsSetT{});
    }

    SECTION("SUB_REQ with empty topic") {
        msg.type = LocalMessageType::SUB_REQ;
        msg.topic = "";
        ll.receiveDirect(msg);

        std::this_thread::sleep_for(10ms);

        CHECK(ll.getSentMsgs() == SentMsgsSetT{});
        CHECK(fl.getSubs() == SubsSetT{});
        CHECK(fl.getSubsLog() == SubsLogT{});
    }
}

TEST_CASE("Receive from far layer with expiration", "[Bridge]") {
    LocalLayers::DummyLocalLayer ll{};
    FarLayers::DummyFarLayer fl{};
    Nodes::Bridge br{&ll, &fl, CONF};

    bool localSub1Passed = false;
    bool localSub2Passed = false;

    auto localSub1Cb = [&localSub1Passed](const std::string& topic,
                                          const std::string& payload)
    {
        CHECK(topic == TOPIC);
        CHECK(payload == PAYLOAD);
        localSub1Passed = true;
    };

    auto localSub2Cb = [&localSub2Passed](const std::string& topic,
                                          const std::string& payload)
    {
        CHECK(topic == TOPIC_SUFFIX);
        CHECK(payload == PAYLOAD);
        localSub2Passed = true;
    };

    // Subscribe locally
    REQUIRE(br.subscribe(TOPIC, localSub1Cb));
    REQUIRE(br.subscribe(TOPIC_ML_WILD, localSub2Cb));

    // Simulate subscription from local layer
    auto llSub11 = MSG_SUB1;
    auto llSub21 = MSG_SUB2;
    auto llSub22 = MSG_SUB2;
    llSub11.topic = TOPIC_SL_WILD;
    llSub21.topic = TOPIC_ML_WILD;
    llSub22.topic = TOPIC_SUFFIX;
    ll.receiveDirect(llSub11);
    ll.receiveDirect(llSub21);
    ll.receiveDirect(llSub22);

    SECTION("Receive data for `TOPIC`") {
        // Simulate data from far layer
        fl.receiveDirect(TOPIC, PAYLOAD);

        // Leave some room for propagation from thread
        std::this_thread::sleep_for(10ms);

        CHECK(localSub1Passed);
        CHECK(!localSub2Passed);
        CHECK(ll.getSentMsgs() == SentMsgsSetT{});
    }

    SECTION("Receive data for `TOPIC_SUFFIX`") {
        // Simulate data from far layer
        fl.receiveDirect(TOPIC_SUFFIX, PAYLOAD);

        // Leave some room for propagation from thread
        std::this_thread::sleep_for(10ms);

        CHECK(!localSub1Passed);
        CHECK(localSub2Passed);
        CHECK(ll.getSentMsgs() == SentMsgsSetT{
            {
                .type = LocalMessageType::SUB_DATA,
                .addr = llSub11.addr,
                .topic = TOPIC_SUFFIX,
                .payload = PAYLOAD,
            },
            {
                .type = LocalMessageType::SUB_DATA,
                .addr = llSub21.addr,
                .topic = TOPIC_SUFFIX,
                .payload = PAYLOAD},
            {
                .type = LocalMessageType::SUB_DATA,
                .addr = llSub22.addr,
                .topic = TOPIC_SUFFIX,
                .payload = PAYLOAD,
            },
        });
    }

    std::this_thread::sleep_for(CONF.subDB.subLifetime
                                + 2*CONF.subDB.interval);

    // Only this-node subscriptions should be left
    CHECK(fl.getSubs() == SubsSetT{TOPIC, TOPIC_ML_WILD});
}
