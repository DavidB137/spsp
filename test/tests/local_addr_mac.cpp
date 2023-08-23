#include <catch2/catch_test_macros.hpp>

#include "spsp_local_addr_mac.hpp"

using namespace SPSP;

TEST_CASE("Default constructor creates empty address", "[LocalAddrMAC]") {
	REQUIRE(LocalAddrMAC() == LocalAddrMAC::zeroes());
}
