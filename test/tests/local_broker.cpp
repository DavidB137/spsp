#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <string>
#include <thread>

#include "spsp_local_broker.hpp"
#include "spsp_node.hpp"

using namespace SPSP;
using namespace std::chrono_literals;

const std::string SRC = "549b3d00da16ca2d";
const std::string TOPIC_PREFIX = "spsp";
const std::string TOPIC = "abc";
const std::string TOPIC_FOR_WILDCARD = "111/abc";
const std::string PAYLOAD = "123";
const std::string TOPIC_PUBLISH = TOPIC_PREFIX + "/" + SRC + "/" + TOPIC;
const std::string TOPIC_PUBLISH_WILDCARD = TOPIC_PREFIX + "/" + SRC + "/" + TOPIC_FOR_WILDCARD;
const std::string TOPIC_WILDCARD = TOPIC_PREFIX + "/" + SRC + "/+/" + TOPIC;

class Node : IFarNode<FarLayers::LocalBroker::LocalBroker>
{
public:
    bool called = false;
    std::string receivedTopic;
    std::string receivedPayload;

    using IFarNode::IFarNode;

    virtual bool publish(const std::string& topic,
                         const std::string& payload)
    {
        return true;
    }

    virtual bool subscribe(const std::string& topic, SubscribeCb cb)
    {
        return true;
    }

    virtual bool unsubscribe(const std::string& topic) {
        return true;
    }

    virtual bool receiveFar(const std::string& topic,
                            const std::string& payload)
    {
        called = true;
        receivedTopic = topic;
        receivedPayload = payload;
        return true;
    }

    virtual void resubscribeAll() {}
};

TEST_CASE("Check return values", "[LocalBroker]") {
    FarLayers::LocalBroker::LocalBroker lb{TOPIC_PREFIX};

    SECTION("Publish") {
        REQUIRE(lb.publish(SRC, TOPIC, PAYLOAD));
    }

    SECTION("Subscribe") {
        REQUIRE(lb.subscribe(TOPIC));
    }

    SECTION("Unsubscribe without preceding subscribe") {
        REQUIRE(!lb.unsubscribe(TOPIC));
    }

    SECTION("Unsubscribe without preceding subscribe - wildcard") {
        REQUIRE(!lb.unsubscribe(TOPIC + "/#"));
    }

    SECTION("Unsubscribe with preceding subscribe") {
        REQUIRE(lb.subscribe(TOPIC));
        REQUIRE(lb.unsubscribe(TOPIC));
    }

    SECTION("Unsubscribe with preceding subscribe - wildcard") {
        REQUIRE(lb.subscribe(TOPIC + "/#"));
        REQUIRE(lb.unsubscribe(TOPIC + "/#"));
    }
}

TEST_CASE("Receive subscription data", "[LocalBroker]") {
    FarLayers::LocalBroker::LocalBroker lb{TOPIC_PREFIX};
    Node node{&lb};

    SECTION("Publish, don't receive") {
        CHECK(lb.publish(SRC, TOPIC, PAYLOAD));
        std::this_thread::sleep_for(10ms);
        CHECK(!node.called);
    }

    SECTION("Subscribe, publish, receive") {
        CHECK(lb.subscribe(TOPIC_PUBLISH));
        CHECK(lb.publish(SRC, TOPIC, PAYLOAD));
        std::this_thread::sleep_for(10ms);
        CHECK(node.called);
        CHECK(node.receivedTopic == TOPIC_PUBLISH);
        CHECK(node.receivedPayload == PAYLOAD);
    }

    SECTION("Subscribe, publish, receive - wildcard") {
        CHECK(lb.subscribe(TOPIC_WILDCARD));
        CHECK(lb.publish(SRC, TOPIC_FOR_WILDCARD, PAYLOAD));
        std::this_thread::sleep_for(10ms);
        CHECK(node.called);
        CHECK(node.receivedTopic == TOPIC_PUBLISH_WILDCARD);
        CHECK(node.receivedPayload == PAYLOAD);
    }

    SECTION("Subscribe, unsubscribe, publish, don't receive") {
        CHECK(lb.subscribe(TOPIC_PUBLISH));
        CHECK(lb.unsubscribe(TOPIC_PUBLISH));
        CHECK(lb.publish(SRC, TOPIC, PAYLOAD));
        std::this_thread::sleep_for(10ms);
        CHECK(!node.called);
    }

    SECTION("Subscribe, unsubscribe, publish, don't receive - wildcard") {
        CHECK(lb.subscribe(TOPIC_WILDCARD));
        CHECK(lb.unsubscribe(TOPIC_WILDCARD));
        CHECK(lb.publish(SRC, TOPIC_FOR_WILDCARD, PAYLOAD));
        std::this_thread::sleep_for(10ms);
        CHECK(!node.called);
    }
}

TEST_CASE("Check empty topic prefix", "[LocalBroker]") {
    FarLayers::LocalBroker::LocalBroker lb{""};
    Node node{&lb};

    CHECK(lb.subscribe(SRC + "/" + TOPIC));
    CHECK(lb.publish(SRC, TOPIC, PAYLOAD));
    std::this_thread::sleep_for(10ms);
    CHECK(node.called);
    CHECK(node.receivedTopic == SRC + "/" + TOPIC);
    CHECK(node.receivedPayload == PAYLOAD);
}
