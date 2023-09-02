#include <catch2/catch_test_macros.hpp>

#include <unordered_map>
#include <vector>

#include "spsp_wildcard_trie.hpp"

using namespace SPSP;

using FindReturnT = SPSP::WildcardTrie<int>::FindReturnT;

TEST_CASE("Simple insert, remove, find in wildcard trie", "[WildcardTrie]") {
    WildcardTrie<int> trie("/", "+", "#");

    REQUIRE(trie.empty());

    SECTION("Simple insert") {
        trie.insert("abc/def", 2);
        REQUIRE(!trie.empty());
    }

    SECTION("Remove non-existing") {
        REQUIRE(!trie.remove("abc/def"));
    }

    SECTION("Find non-existing") {
        REQUIRE(trie.find("abc/def").empty());
    }

    SECTION("Find empty key") {
        REQUIRE(trie.find("").empty());
    }
}

TEST_CASE("Insert and find in wildcard trie", "[WildcardTrie]") {
    WildcardTrie<int> trie("/", "+", "#");

    REQUIRE(trie.empty());

    SECTION("Insert and find simple") {
        trie.insert("abc/def", 2);
        REQUIRE(trie.find("abc/def") == FindReturnT{{"abc/def", 2}});
        REQUIRE(trie.find("abc/def0").empty());
    }

    SECTION("Insert and find simple rewrite") {
        trie.insert("abc/def", 2);
        REQUIRE(trie.find("abc/def") == FindReturnT{{"abc/def", 2}});
        
        trie.insert("abc/def", 3);
        REQUIRE(trie.find("abc/def") == FindReturnT{{"abc/def", 3}});
    }

    SECTION("Insert and find empty key") {
        trie.insert("abc", 2);
        REQUIRE(trie.find("").empty());
    }

    SECTION("Insert and find single-level wildcard at the end") {
        trie.insert("abc/+", 2);
        REQUIRE(trie.find("abc/aaa") == FindReturnT{{"abc/+", 2}});
        REQUIRE(trie.find("abc/aaa/1").empty());
        REQUIRE(trie.find("abc").empty());
    }

    SECTION("Insert and find single-level wildcard in the middle") {
        trie.insert("abc/+/def", 2);
        REQUIRE(trie.find("abc/aaa/def") == FindReturnT{{"abc/+/def", 2}});
        REQUIRE(trie.find("abc/aaa/def/1").empty());
        REQUIRE(trie.find("abc/1").empty());
        REQUIRE(trie.find("abc").empty());
    }

    SECTION("Insert and find single-level wildcard at the beginning") {
        trie.insert("+/def", 2);
        REQUIRE(trie.find("abc/def") == FindReturnT{{"+/def", 2}});
        REQUIRE(trie.find("abc/def/1").empty());
        REQUIRE(trie.find("abc").empty());
    }

    SECTION("Insert and find single-level wildcard as only character") {
        trie.insert("+", 2);
        REQUIRE(trie.find("abc") == FindReturnT{{"+", 2}});
        REQUIRE(trie.find("abc/def").empty());
        REQUIRE(trie.find("") == FindReturnT{{"+", 2}});
    }

    SECTION("Insert and find multi-level wildcard at the end") {
        trie.insert("abc/#", 2);
        REQUIRE(trie.find("abc/aaa") == FindReturnT{{"abc/#", 2}});
        REQUIRE(trie.find("abc/aaa/1") == FindReturnT{{"abc/#", 2}});
        REQUIRE(trie.find("abc").empty());
    }

    SECTION("Insert and find multi-level wildcard as only character") {
        trie.insert("#", 2);
        REQUIRE(trie.find("abc") == FindReturnT{{"#", 2}});
        REQUIRE(trie.find("abc/def") == FindReturnT{{"#", 2}});
        REQUIRE(trie.find("") == FindReturnT{{"#", 2}});
    }
}

TEST_CASE("Insert, remove and find in wildcard trie", "[WildcardTrie]") {
    WildcardTrie<int> trie("/", "+", "#");

    REQUIRE(trie.empty());

    SECTION("Insert, remove and find non-existing simple") {
        trie.insert("aaa", 2);
        REQUIRE(!trie.find("aaa").empty());
        REQUIRE(trie.remove("aaa"));
        REQUIRE(trie.find("aaa").empty());
    }

    SECTION("Insert, remove and find non-existing single-level wildcard") {
        trie.insert("aaa/+", 2);
        REQUIRE(!trie.find("aaa/bbb").empty());
        REQUIRE(trie.remove("aaa/+"));
        REQUIRE(trie.find("aaa/bbb").empty());
    }

    SECTION("Insert, remove and find non-existing multi-level wildcard") {
        trie.insert("aaa/#", 2);
        REQUIRE(!trie.find("aaa/bbb/ccc").empty());
        REQUIRE(trie.remove("aaa/#"));
        REQUIRE(trie.find("aaa/bbb/ccc").empty());
    }

    SECTION("Insert, remove and find two keys without common prefix") {
        trie.insert("aaa", 2);
        trie.insert("bbb", 3);

        REQUIRE(!trie.find("aaa").empty());
        REQUIRE(!trie.find("bbb").empty());

        REQUIRE(trie.remove("aaa"));
        REQUIRE(trie.find("aaa").empty());

        REQUIRE(trie.remove("bbb"));
        REQUIRE(trie.find("bbb").empty());
    }

    SECTION("Insert, remove and find two keys with common prefix") {
        trie.insert("aaa/bbb", 2);
        trie.insert("aaa/ccc", 3);

        REQUIRE(!trie.find("aaa/bbb").empty());
        REQUIRE(!trie.find("aaa/ccc").empty());

        // Attempt to remove non-leaf prefix
        REQUIRE(!trie.remove("aaa"));

        REQUIRE(!trie.find("aaa/bbb").empty());
        REQUIRE(!trie.find("aaa/ccc").empty());

        REQUIRE(trie.remove("aaa/bbb"));
        REQUIRE(trie.find("aaa/bbb").empty());
        REQUIRE(!trie.find("aaa/ccc").empty());
        REQUIRE(trie.remove("aaa/ccc"));
        REQUIRE(trie.find("aaa/ccc").empty());
    }

    SECTION("Insert, remove and find keys with common leaf prefix") {
        trie.insert("aaa", 1);
        trie.insert("aaa/bbb", 2);
        trie.insert("aaa/ccc", 3);

        REQUIRE(!trie.find("aaa").empty());
        REQUIRE(!trie.find("aaa/bbb").empty());
        REQUIRE(!trie.find("aaa/ccc").empty());

        REQUIRE(trie.remove("aaa/bbb"));
        REQUIRE(!trie.find("aaa").empty());
        REQUIRE(trie.find("aaa/bbb").empty());
        REQUIRE(!trie.find("aaa/ccc").empty());

        REQUIRE(trie.remove("aaa/ccc"));
        REQUIRE(!trie.find("aaa").empty());
        REQUIRE(trie.find("aaa/bbb").empty());
        REQUIRE(trie.find("aaa/ccc").empty());

        REQUIRE(trie.remove("aaa"));
        REQUIRE(trie.find("aaa").empty());
        REQUIRE(trie.find("aaa/bbb").empty());
        REQUIRE(trie.find("aaa/ccc").empty());
    }

    SECTION("Insert, remove and find keys with wildcards") {
        trie.insert("aaa/+/ccc/#", 2);
        trie.insert("aaa/#", 3);

        REQUIRE(!trie.find("aaa/bbb").empty());
        REQUIRE(!trie.find("aaa/bbb/ccc/ddd").empty());

        REQUIRE(!trie.remove("aaa/bbb"));

        REQUIRE(trie.remove("aaa/#"));
        REQUIRE(!trie.find("aaa/bbb/ccc/ddd").empty());
        REQUIRE(trie.remove("aaa/+/ccc/#"));
        REQUIRE(trie.find("aaa/bbb/ccc/ddd").empty());
    }

    // All tests must clean up
    REQUIRE(trie.empty());
}

TEST_CASE("Find in wildcard trie", "[WildcardTrie]") {
    WildcardTrie<int> trie("/", "+", "#");

    trie.insert("abc/#", 2);
    trie.insert("abc/def", 3);
    trie.insert("abc/def/g", 4);
    trie.insert("abc/def/+/h", 5);
    trie.insert("other/#", 6);
    trie.insert("if/+/else", 7);

    REQUIRE(!trie.empty());

    SECTION("Find non-existing prefix") {
        REQUIRE(trie.find("abc").empty());
    }

    SECTION("Find non-existing long") {
        REQUIRE(trie.find("something/123").empty());
    }

    SECTION("Find non-existing long with single-level wildcard") {
        REQUIRE(trie.find("if/abc/else/aaa").empty());
    }

    SECTION("Find simple") {
        REQUIRE(trie.find("abc/def").size() == 2);
        REQUIRE(trie.find("abc/def/g").size() == 2);
    }

    SECTION("Find single-level wildcard") {
        REQUIRE(trie.find("if/elseif/else") == FindReturnT{{"if/+/else", 7}});
    }

    SECTION("Find multi-level wildcard") {
        REQUIRE(trie.find("other/123") == FindReturnT{{"other/#", 6}});
    }

    SECTION("Find multiple wildcards") {
        REQUIRE(trie.find("abc/def/xyz/h").size() == 2);
    }
}

TEST_CASE("For each and [] in wildcard trie", "[WildcardTrie]") {
    WildcardTrie<int> trie("/", "+", "#");

    trie.insert("abc/#", 2);
    trie.insert("abc/def", 3);
    trie.insert("abc/def/g", 4);
    trie.insert("abc/def/+/h", 5);
    trie.insert("other/#", 6);
    trie.insert("if/+/else", 7);

    REQUIRE(!trie.empty());

    std::unordered_map<std::string, int> values;

    trie.forEach([&trie, &values](const std::string& key, const int& value) {
        // Somehow modify value
        trie[key] = value + 1;

        // Store keys and values
        values[key] = value;
    });

    REQUIRE(values == std::unordered_map<std::string, int>{
        {"abc/#", 3},
        {"abc/def", 4},
        {"abc/def/g", 5},
        {"abc/def/+/h", 6},
        {"other/#", 7},
        {"if/+/else", 8},
    });

    // Check value was modified
    REQUIRE(trie.find("if/1/else") == FindReturnT{{"if/+/else", 8}});
}
