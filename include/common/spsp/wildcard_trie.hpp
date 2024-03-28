/**
 * @file wildcard_trie.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Trie implementation with wildcard support
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <functional>
#include <memory>
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
     * Multi-level wildcard must be the last character in the key.
     * There are no exceptions and no key validation. If key is
     * semantically invalid, the item will just become inaccessible.
     *
     * @tparam TValue Type of value
     */
    template <typename TValue>
    class WildcardTrie
    {
        /**
         * @brief Internal node of wildcard trie
         *
         */
        struct Node
        {
            TValue value;                                                   //!< Value
            std::unordered_map<std::string, std::unique_ptr<Node>> childs;  //!< Children
            size_t levelIndex = 0;                                          //!< Index of level
            bool isLeaf = false;                                            //!< Whether is leaf node
        };

        using BFSQueueT = std::queue<std::pair<std::string, const Node*>>;

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
        WildcardTrie(const std::string& levelSeparator = "/",
                     const std::string& singleLevelWildcard = "+",
                     const std::string& multiLevelWildcard = "#") noexcept
            : m_lSep{levelSeparator}, m_lSingleWild{singleLevelWildcard},
              m_lMultiWild{multiLevelWildcard}
        {}

        /**
         * @brief Gets/inserts current value of `key`
         *
         * @param key Key
         * @return Current value reference
         */
        TValue& operator[](const std::string& key)
        {
            Node* cur = &m_root;
            auto levels = this->splitToLevels(key);

            // Get or create child on each level
            for (size_t i = 0; i < levels.size(); i++) {
                auto& level = levels[i];

                // Create new child
                if (cur->childs.find(level) == cur->childs.end()) {
                    cur->childs[level] = std::make_unique<Node>();
                    cur->childs[level]->levelIndex = i + 1;
                }

                // Move to next level
                cur = cur->childs.at(level).get();
            }

            cur->isLeaf = true;

            return cur->value;
        }

        /**
         * @brief Inserts (or updates) `key`-`value` pair
         *
         * @param key Key
         * @param value Value
         */
        void insert(const std::string& key, const TValue& value)
        {
            (*this)[key] = value;
        }

        /**
         * @brief Removes `key` from trie
         *
         * @param key Key
         * @return true Node removed successfully
         * @return false Node doesn't exist
         */
        bool remove(const std::string& key)
        {
            Node* cur = &m_root;
            auto levels = this->splitToLevels(key);

            std::vector<Node*> nodeStack;

            // Get node if exists
            for (auto& level : levels) {
                nodeStack.push_back(cur);

                if (cur->childs.find(level) == cur->childs.end()) {
                    return false;
                }
                cur = cur->childs.at(level).get();
            }

            // Can't remove non-leaf node
            if (!cur->isLeaf) {
                return false;
            }

            cur->isLeaf = false;

            if (cur->childs.empty()) {
                // Delete all redundant ancestors
                // There is `int` instead of `size_t`, because we need signed type.
                for (int i = nodeStack.size() - 1; i >= 0; i--) {
                    Node* node = nodeStack.at(i);
                    if (node->isLeaf || node->childs.size() > 1 ||
                        node == &m_root) {
                        node->childs.erase(levels.at(i));

                        // Previous ancestors are no longer redundant 
                        break;
                    }
                }
            }

            return true;
        }

        using FindReturnT = std::unordered_map<std::string, const TValue&>;

        /**
         * @brief Finds `key` in trie
         *
         * @param key Key
         * @return Vector of values from matching keys (empty if not found)
         */
        const FindReturnT find(const std::string& key) const
        {
            auto levels = this->splitToLevels(key);

            FindReturnT values;

            // Queue for to-be-processed nodes
            BFSQueueT nodeQueue;
            nodeQueue.push({ "", &m_root });

            while (!nodeQueue.empty()) {
                auto [nodeKey, node] = nodeQueue.front();

                if (node->levelIndex == levels.size() && node->isLeaf) {
                    // Match
                    values.insert({ nodeKey, node->value });
                }
                else if (node->levelIndex < levels.size()) {
                    // Enqueue relevant childs
                    for (auto& [childLevel, childNode] : node->childs) {
                        std::string childKey = nodeKey == ""
                            ? childLevel
                            : nodeKey + m_lSep + childLevel;

                        if (childLevel == levels.at(node->levelIndex) ||
                            childLevel == m_lSingleWild) {
                            // Key matches or has single-level wildcard
                            nodeQueue.push({ childKey, childNode.get() });
                        }
                        else if (childLevel == m_lMultiWild &&
                                 childNode->isLeaf) {
                            // Multi-level wildcard
                            values.insert({ childKey, childNode->value });
                        }
                    }
                }

                nodeQueue.pop();
            }

            return values;
        }

        /**
         * @brief Iterates through each item in trie and calls callback
         *        on each one
         *
         * @param f Function to call
         */
        void forEach(std::function<void(const std::string& key, const TValue& value)> f)
        {
            // Queue for to-be-processed nodes
            BFSQueueT nodeQueue;
            nodeQueue.push({ "", &m_root });

            while (!nodeQueue.empty()) {
                auto [nodeKey, node] = nodeQueue.front();

                // Call function
                if (node->isLeaf) {
                    f(nodeKey, node->value);
                }

                // Enqueue children
                for (auto& [childLevel, childNode] : node->childs) {
                    std::string childKey = nodeKey == ""
                        ? childLevel
                        : nodeKey + m_lSep + childLevel;
                    nodeQueue.push({ childKey, childNode.get() });
                }

                nodeQueue.pop();
            }
        }

        /**
         * @brief Empty predicate
         *
         * @return true Trie is empty
         * @return false Trie is not empty
         */
        bool empty() const
        {
            return m_root.childs.empty();
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

            while ((nextPos = key.find(m_lSep, curPos)) != std::string::npos) {
                levels.push_back(key.substr(curPos, nextPos - curPos));
                curPos = nextPos + m_lSep.length();
            }

            // Add the rest
            levels.push_back(key.substr(curPos));

            return levels;
        }
    };
} // namespace SPSP
