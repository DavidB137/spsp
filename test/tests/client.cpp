#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "spsp_client.hpp"
#include "spsp_layers_dummy.hpp"
#include "spsp_local_addr.hpp"
#include "spsp_local_message.hpp"
#include "spsp_version.hpp"

using namespace SPSP;
using namespace std::chrono_literals;

using PubsSetT = typename FarLayers::DummyFarLayer::PubsSetT;
using SubsSetT = typename FarLayers::DummyFarLayer::SubsSetT;
using SubsLogT = typename FarLayers::DummyFarLayer::SubsLogT;
using SentMsgsSetT = typename LocalLayers::DummyLocalLayer::SentMsgsSetT;

const LocalAddr ADDR_PEER = { .addr = {0, 0, 0, 1}, .str = "0001" };
const std::string TOPIC = "abc";
const std::string TOPIC_SUFFIX = "abc/def";
const std::string TOPIC_SL_WILD = "abc/+";
const std::string TOPIC_ML_WILD = "abc/#";
const std::string PAYLOAD = "123";
const Nodes::ClientConfig CONF = {
    .reporting = {
        .rssiOnProbe = false,
    },
    .subDB = {
        .interval = 10ms,
        .subLifetime = 100ms,
    },
};
const LocalMessage<LocalAddr> MSG = {
    .type = LocalMessageType::PUB,
    .addr = LocalAddr{},
    .topic = TOPIC,
    .payload = PAYLOAD
};

TEST_CASE("Publish", "[Client]") {
    LocalLayers::DummyLocalLayer ll{};
    Nodes::Client cl{&ll, CONF};

    CHECK(cl.publish(TOPIC, PAYLOAD));
    CHECK(ll.getSentMsgs() == SentMsgsSetT{{
        .type = LocalMessageType::PUB,
        .addr = LocalAddr{},
        .topic = TOPIC,
        .payload = PAYLOAD,
    }});
}

TEST_CASE("Subscribe", "[Client]") {
    LocalLayers::DummyLocalLayer ll{};
    Nodes::Client cl{&ll, CONF};

    SECTION("Single topic (and check periodic resubscribe)") {
        REQUIRE(cl.subscribe(TOPIC, nullptr));

        std::this_thread::sleep_for(2*CONF.subDB.subLifetime
                                    + 2*CONF.subDB.interval);

        CHECK(ll.getSentMsgsCount() == 3);
        CHECK(ll.getSentMsgs() == SentMsgsSetT{{
            .type = LocalMessageType::SUB_REQ,
            .addr = LocalAddr{},
            .topic = TOPIC,
            .payload = ""
        }});
    }

    SECTION("Two different topics") {
        REQUIRE(cl.subscribe(TOPIC, nullptr));
        REQUIRE(cl.subscribe(TOPIC_SUFFIX, nullptr));

        CHECK(ll.getSentMsgsCount() == 2);
        CHECK(ll.getSentMsgs() == SentMsgsSetT{
            {
                .type = LocalMessageType::SUB_REQ,
                .addr = LocalAddr{},
                .topic = TOPIC,
                .payload = ""
            },
            {
                .type = LocalMessageType::SUB_REQ,
                .addr = LocalAddr{},
                .topic = TOPIC_SUFFIX,
                .payload = ""
            },
        });
    }

    SECTION("Twice to the same topic") {
        REQUIRE(cl.subscribe(TOPIC, nullptr));
        REQUIRE(cl.subscribe(TOPIC, nullptr));

        CHECK(ll.getSentMsgsCount() == 2);
        CHECK(ll.getSentMsgs() == SentMsgsSetT{{
            .type = LocalMessageType::SUB_REQ,
            .addr = LocalAddr{},
            .topic = TOPIC,
            .payload = ""
        }});
    }

    SECTION("Single level wildcard topic") {
        REQUIRE(cl.subscribe(TOPIC_SL_WILD, nullptr));

        CHECK(ll.getSentMsgsCount() == 1);
        CHECK(ll.getSentMsgs() == SentMsgsSetT{{
            .type = LocalMessageType::SUB_REQ,
            .addr = LocalAddr{},
            .topic = TOPIC_SL_WILD,
            .payload = ""
        }});
    }

    SECTION("Multi level wildcard topic") {
        REQUIRE(cl.subscribe(TOPIC_ML_WILD, nullptr));

        CHECK(ll.getSentMsgsCount() == 1);
        CHECK(ll.getSentMsgs() == SentMsgsSetT{{
            .type = LocalMessageType::SUB_REQ,
            .addr = LocalAddr{},
            .topic = TOPIC_ML_WILD,
            .payload = ""
        }});
    }

    SECTION("Empty topic") {
        REQUIRE(!cl.subscribe("", nullptr));

        CHECK(ll.getSentMsgsCount() == 0);
    }
}

TEST_CASE("Unsubscribe", "[Client]") {
    LocalLayers::DummyLocalLayer ll{};
    Nodes::Client cl{&ll, CONF};

    REQUIRE(cl.subscribe(TOPIC, nullptr));
    REQUIRE(cl.subscribe(TOPIC_SUFFIX, nullptr));
    REQUIRE(cl.subscribe(TOPIC_SL_WILD, nullptr));
    REQUIRE(cl.subscribe(TOPIC_ML_WILD, nullptr));

    auto msg_sub_base = MSG;
    msg_sub_base.type = LocalMessageType::SUB_REQ;
    msg_sub_base.payload = "";

    auto msg_sub1 = msg_sub_base, msg_sub2 = msg_sub_base,
         msg_sub3 = msg_sub_base, msg_sub4 = msg_sub_base;
    msg_sub1.topic = TOPIC;
    msg_sub2.topic = TOPIC_SUFFIX;
    msg_sub3.topic = TOPIC_SL_WILD;
    msg_sub4.topic = TOPIC_ML_WILD;

    SECTION("Simple topic") {
        REQUIRE(cl.unsubscribe(TOPIC));

        CHECK(ll.getSentMsgsCount() == 5);
        CHECK(ll.getSentMsgs() == SentMsgsSetT{
            msg_sub1, msg_sub2, msg_sub3, msg_sub4,
            {
                .type = LocalMessageType::UNSUB,
                .addr = LocalAddr{},
                .topic = TOPIC,
                .payload = ""
            }
        });
    }

    SECTION("Topic and it's prefix") {
        REQUIRE(cl.unsubscribe(TOPIC_SUFFIX));
        REQUIRE(cl.unsubscribe(TOPIC));

        CHECK(ll.getSentMsgsCount() == 6);
        CHECK(ll.getSentMsgs() == SentMsgsSetT{
            msg_sub1, msg_sub2, msg_sub3, msg_sub4,
            {
                .type = LocalMessageType::UNSUB,
                .addr = LocalAddr{},
                .topic = TOPIC_SUFFIX,
                .payload = ""
            },
            {
                .type = LocalMessageType::UNSUB,
                .addr = LocalAddr{},
                .topic = TOPIC,
                .payload = ""
            },
        });
    }

    SECTION("Single level wildcard") {
        REQUIRE(cl.unsubscribe(TOPIC_SL_WILD));

        CHECK(ll.getSentMsgsCount() == 5);
        CHECK(ll.getSentMsgs() == SentMsgsSetT{
            msg_sub1, msg_sub2, msg_sub3, msg_sub4,
            {
                .type = LocalMessageType::UNSUB,
                .addr = LocalAddr{},
                .topic = TOPIC_SL_WILD,
                .payload = ""
            },
        });
    }

    SECTION("Multi level wildcard") {
        REQUIRE(cl.unsubscribe(TOPIC_ML_WILD));

        CHECK(ll.getSentMsgsCount() == 5);
        CHECK(ll.getSentMsgs() == SentMsgsSetT{
            msg_sub1, msg_sub2, msg_sub3, msg_sub4,
            {
                .type = LocalMessageType::UNSUB,
                .addr = LocalAddr{},
                .topic = TOPIC_ML_WILD,
                .payload = ""
            },
        });
    }

    SECTION("All subscribed topics in random order") {
        REQUIRE(cl.unsubscribe(TOPIC_SUFFIX));
        REQUIRE(cl.unsubscribe(TOPIC_ML_WILD));
        REQUIRE(cl.unsubscribe(TOPIC));
        REQUIRE(cl.unsubscribe(TOPIC_SL_WILD));

        CHECK(ll.getSentMsgsCount() == 8);
        CHECK(ll.getSentMsgs() == SentMsgsSetT{
            msg_sub1, msg_sub2, msg_sub3, msg_sub4,
            {
                .type = LocalMessageType::UNSUB,
                .addr = LocalAddr{},
                .topic = TOPIC_SUFFIX,
                .payload = ""
            },
            {
                .type = LocalMessageType::UNSUB,
                .addr = LocalAddr{},
                .topic = TOPIC_ML_WILD,
                .payload = ""
            },
            {
                .type = LocalMessageType::UNSUB,
                .addr = LocalAddr{},
                .topic = TOPIC,
                .payload = ""
            },
            {
                .type = LocalMessageType::UNSUB,
                .addr = LocalAddr{},
                .topic = TOPIC_SL_WILD,
                .payload = ""
            },
        });
    }

    SECTION("Non-existing topic") {
        REQUIRE(!cl.unsubscribe(TOPIC + "x"));

        CHECK(ll.getSentMsgsCount() == 4);
        CHECK(ll.getSentMsgs() == SentMsgsSetT{
            msg_sub1, msg_sub2, msg_sub3, msg_sub4,
        });
    }

    SECTION("Empty topic") {
        REQUIRE(!cl.unsubscribe(""));

        CHECK(ll.getSentMsgsCount() == 4);
        CHECK(ll.getSentMsgs() == SentMsgsSetT{
            msg_sub1, msg_sub2, msg_sub3, msg_sub4,
        });
    }
}

TEST_CASE("Unsubscribe actually removes topic from resubscribing", "[Client]") {
    LocalLayers::DummyLocalLayer ll{};
    Nodes::Client cl{&ll, CONF};

    REQUIRE(cl.subscribe(TOPIC, nullptr));
    REQUIRE(cl.unsubscribe(TOPIC));

    std::this_thread::sleep_for(2*CONF.subDB.subLifetime
                                + 2*CONF.subDB.interval);

    CHECK(ll.getSentMsgsCount() == 2);
    CHECK(ll.getSentMsgs() == SentMsgsSetT{
        {
            .type = LocalMessageType::SUB_REQ,
            .addr = LocalAddr{},
            .topic = TOPIC,
            .payload = ""
        },
        {
            .type = LocalMessageType::UNSUB,
            .addr = LocalAddr{},
            .topic = TOPIC,
            .payload = ""
        }
    });
}

TEST_CASE("Receive from local layer", "[Client]") {
    LocalLayers::DummyLocalLayer ll{};
    Nodes::Client cl{&ll, CONF};

    auto msg = MSG;
    msg.type = LocalMessageType::PROBE_REQ;

    SECTION("PROBE_REQ") {
        msg.type = LocalMessageType::PROBE_REQ;
    }

    SECTION("PROBE_RES") {
        msg.type = LocalMessageType::PROBE_RES;
    }

    SECTION("PUB") {
        msg.type = LocalMessageType::PUB;
    }

    SECTION("SUB_REQ") {
        msg.type = LocalMessageType::SUB_REQ;
    }

    SECTION("SUB_DATA") {
        msg.type = LocalMessageType::SUB_DATA;
    }

    SECTION("UNSUB") {
        msg.type = LocalMessageType::UNSUB;
    }

    ll.receiveDirect(msg);

    std::this_thread::sleep_for(10ms);

    CHECK(ll.getSentMsgs() == SentMsgsSetT{});
}

TEST_CASE("Receive subscription data", "[Client]") {
    LocalLayers::DummyLocalLayer ll{};
    Nodes::Client cl{&ll, CONF};

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
    REQUIRE(cl.subscribe(TOPIC, localSub1Cb));
    REQUIRE(cl.subscribe(TOPIC_ML_WILD, localSub2Cb));

    auto msg = MSG;
    msg.type = LocalMessageType::SUB_DATA;

    SECTION("Receive data for `TOPIC`") {
        // Simulate subscription data
        msg.topic = TOPIC;
        ll.receiveDirect(msg);

        // Leave some room for propagation from thread
        std::this_thread::sleep_for(10ms);

        CHECK(localSub1Passed);
        CHECK(!localSub2Passed);
    }

    SECTION("Receive data for `TOPIC_SUFFIX`") {
        // Simulate subscription data
        msg.topic = TOPIC_SUFFIX;
        ll.receiveDirect(msg);

        // Leave some room for propagation from thread
        std::this_thread::sleep_for(10ms);

        CHECK(!localSub1Passed);
        CHECK(localSub2Passed);
    }
}
