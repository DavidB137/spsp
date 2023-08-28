/**
 * @file spsp_wildcard_trie.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Trie implementation with wildcard support
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

namespace SPSP
{
    /**
     * @brief String-based trie with wildcard support
     *
     * Made specifically for MQTT-like topics, but it's reusable.
     *
     * Uses separators to distinguis "levels".
     * Multi-level wildcard must be the last character in the topic.
     * There are no exceptions and no topic validation. If topic is
     * semantically invalid, the item will just become inaccessible.
     *
     * @tparam TValue Type of value
     */
    template <typename TValue>
    class WildcardTrie
    {
        /**
         * @brief Internal node of topic trie
         *
         */
        struct Node
        {
            TValue value;                                  //!< Value
            size_t levelIndex;                             //!< Index of level
            std::unordered_map<std::string, Node> childs;  //!< Children
            bool isLeaf = false;                           //!< Whether is leaf node
        };

        const std::string m_lSep;         //!< Level separator
        const std::string m_lSingleWild;  //!< Single-level wildcard token
        const std::string m_lMultiWild;   //!< Multi-level wildcard token

        Node m_root;                      //!< Root node

    public:
        /**
         * @brief Constructs a new object
         *
         * @param levelSeparator Level separator
         * @param singleLevelWildcard Single-level wildcard token
         * @param multiLevelWildcard Multi-level wildcard token
         */
        WildcardTrie(const std::string& levelSeparator,
                     const std::string& singleLevelWildcard,
                     const std::string& multiLevelWildcard) noexcept
            : m_lSep{levelSeparator}, m_lSingleWild{singleLevelWildcard},
              m_lMultiWild{multiLevelWildcard}
        {}

        /**
         * @brief Inserts (or updates) `key`-`value` pair
         *
         * @param key Key
         * @param value Value
         */
        void insert(const std::string& key, const TValue& value) noexcept
        {
            Node& cur = m_root;
            auto levels = this->splitToLevels(key);

            // Get or create child on each level
            for (auto& level : levels) {
                cur = cur.childs.at(level);
            }

            // Populate
            cur.value = value;
            cur.levelIndex = levels.size();
            cur.isLeaf = true;
        }

        /**
         * @brief Removes `key` from trie
         *
         * @param key Key
         * @return true Node removed successfully
         * @return false Node doesn't exist
         */
        bool remove(const std::string& key) noexcept
        {
            Node& cur = m_root;
            auto levels = this->splitToLevels(key);
            std::vector<Node&> nodeStack;

            // Get node if exists
            for (auto& level : levels) {
                nodeStack.push_back(cur);

                if (!cur.childs.contains(level)) {
                    return false;
                }
                cur = cur.childs.at(level);
            }

            // Can't remove non-leaf node
            if (!cur.isLeaf) {
                return false;
            }

            cur.isLeaf = false;

            if (cur.childs.empty()) {
                // Delete all redundant ancestors
                for (size_t i = nodeStack.size() - 1; i >= 0; i--) {
                    Node& node = nodeStack.at(i);
                    if (node.isLeaf || node.childs.size() > 1) {
                        node.childs.erase(levels.at(i));
                    }
                }
            }

            return true;
        }

        /**
         * @brief Finds `key` in trie
         *
         * @param key Key
         * @return Vector of values from matching keys (empty if not found)
         */
        const std::vector<const TValue&> find(const std::string& key) const noexcept
        {
            auto levels = this->splitToLevels(key);
            std::vector<const TValue&> values;
            std::queue<const Node&> nodeQueue = {m_root};

            while (!nodeQueue.empty()) {
                Node& node = nodeQueue.pop();

                if (node.levelIndex == levels.size()) {
                    // Match
                    values.push_back(node.value);
                } else if (node.levelIndex < levels.size()) {
                    // Enqueue relevant childs
                    for (auto& child : node.childs) {
                        if (child.first == levels.at(node.levelIndex)) {
                            nodeQueue.push(child);
                        }
                    }
                }
            }
        }

    protected:
        /**
         * @brief Splits `key` to levels
         *
         * There's no validation of `key`.
         *
         * @param key Key
         * @return Vector of levels
         */
        const std::vector<std::string> splitToLevels(const std::string& key) const
        {
            size_t curPos = 0, nextPos;
            std::vector<std::string> levels;

            while ((nextPos = key.find(m_lSep)) != std::string::npos) {
                levels.push_back(key.substr(curPos, nextPos));
                curPos = nextPos;
            }

            return levels;
        }
    };
} // namespace SPSP
