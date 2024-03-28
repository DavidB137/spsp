#include <catch2/catch_test_macros.hpp>

#include "spsp/local_addr_mac.hpp"
#include "spsp/mac.hpp"

using namespace SPSP;

constexpr uint8_t MAC_LENGTH = 6;

static uint8_t randomMac[MAC_LENGTH] = {1, 2, 3, 10, 5, 0xFF};
static std::string randomMacStr = "0102030a05ff";

TEST_CASE("Empty constructor", "[LocalAddrMAC]") {
    auto addr = LocalAddrMAC();

    REQUIRE(addr.addr == std::vector<uint8_t>{0, 0, 0, 0, 0, 0});
    REQUIRE(addr.str == "000000000000");
}

TEST_CASE("Parametrized constructor", "[LocalAddrMAC]") {
    auto addr = LocalAddrMAC(randomMac);

    REQUIRE(addr.addr == std::vector<uint8_t>(randomMac, randomMac + MAC_LENGTH));
    REQUIRE(addr.str == randomMacStr);
}

TEST_CASE("Local address", "[LocalAddrMAC]") {
    uint8_t mac[MAC_LENGTH] = {};
    getLocalMAC(mac);

    auto addr = LocalAddrMAC::local();

    REQUIRE(addr.addr == std::vector<uint8_t>(mac, mac + MAC_LENGTH));
}

TEST_CASE("Zeroes address", "[LocalAddrMAC]") {
    uint8_t mac[MAC_LENGTH] = {};
    auto addr = LocalAddrMAC::zeroes();

    REQUIRE(addr.addr == std::vector<uint8_t>(mac, mac + MAC_LENGTH));
    REQUIRE(addr.str == "000000000000");
}

TEST_CASE("Broadcast address", "[LocalAddrMAC]") {
    uint8_t mac[MAC_LENGTH] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    auto addr = LocalAddrMAC::broadcast();

    REQUIRE(addr.addr == std::vector<uint8_t>(mac, mac + MAC_LENGTH));
    REQUIRE(addr.str == "ffffffffffff");
}

TEST_CASE("Not empty", "[LocalAddrMAC]") {
    REQUIRE(!LocalAddrMAC().empty());
    REQUIRE(!LocalAddrMAC(randomMac).empty());
    REQUIRE(!LocalAddrMAC::local().empty());
    REQUIRE(!LocalAddrMAC::zeroes().empty());
    REQUIRE(!LocalAddrMAC::broadcast().empty());
}

TEST_CASE("Operator ==", "[LocalAddrMAC]") {
    REQUIRE(static_cast<LocalAddr>(LocalAddrMAC()) == static_cast<LocalAddr>(LocalAddrMAC::zeroes()));
    REQUIRE(static_cast<LocalAddr>(LocalAddrMAC(randomMac)) == static_cast<LocalAddr>(LocalAddrMAC(randomMac)));
    REQUIRE(static_cast<LocalAddr>(LocalAddrMAC::local()) == static_cast<LocalAddr>(LocalAddrMAC::local()));
    REQUIRE_FALSE(static_cast<LocalAddr>(LocalAddrMAC::local()) == static_cast<LocalAddr>(LocalAddrMAC::zeroes()));
    REQUIRE_FALSE(static_cast<LocalAddr>(LocalAddrMAC::local()) == static_cast<LocalAddr>(LocalAddrMAC::broadcast()));
}

TEST_CASE("To MAC", "[LocalAddrMAC]") {
    uint8_t mac2[MAC_LENGTH] = {};

    auto addr1 = LocalAddrMAC(randomMac);
    addr1.toMAC(mac2);
    auto addr2 = LocalAddrMAC(mac2);

    REQUIRE(static_cast<LocalAddr>(addr1) == static_cast<LocalAddr>(addr2));
    REQUIRE(addr1.addr == addr2.addr);
    REQUIRE(addr1.str == addr2.str);
}
