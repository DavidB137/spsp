/**
 * @file spsp_client_sub_db.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Client subscribe database of SPSP
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <chrono>
#include <mutex>
#include <unordered_map>

#include "spsp_logger.hpp"
#include "spsp_node.hpp"
#include "spsp_timer.hpp"

// Log tag
#define SPSP_LOG_TAG "SPSP/Client/SubDB"

namespace SPSP::Nodes
{
    static const uint8_t CLIENT_SUB_LIFETIME = 10;  //!< Default subscribe lifetime

    // Forward declaration
    template <typename TLocalLayer> class Client;

    /**
     * @brief Container for subscribe database of client
     *
     * @tparam TLocalLayer Type of local layer
     */
    template <typename TLocalLayer>
    class ClientSubDB
    {
        /**
         * @brief Client subscribe entry
         *
         * Single entry in subscribe database of a client.
         */
        struct Entry
        {
            uint8_t lifetime = CLIENT_SUB_LIFETIME;  //!< Lifetime in minutes
            SPSP::SubscribeCb cb = nullptr;          //!< Callback for incoming data
        };

        Client<TLocalLayer>* m_client;                //!< Pointer to client (owner)
        std::mutex m_mutex;                           //!< Mutex to prevent race conditions
        Timer m_timer;                                //!< Timer
        std::unordered_map<std::string, Entry> m_db;  //!< Database

    public:
        /**
         * @brief Construct a new client sub DB
         *
         * @param client Pointer to client node (owner)
         */
        ClientSubDB(Client<TLocalLayer>* client)
            : m_client{client},
              m_timer{std::chrono::minutes(1),
                      std::bind(&ClientSubDB<TLocalLayer>::tick, this)},
              m_db{}
        {
            SPSP_LOGD("Initialized");
        }

        /**
         * @brief Destroys the client sub DB
         */
        ~ClientSubDB()
        {
            SPSP_LOGD("Deinitialized");
        }

        /**
         * @brief Inserts entry into database
         *
         * @param topic Topic
         * @param cb Callback for incoming data
         */
        void insert(const std::string topic, SPSP::SubscribeCb cb)
        {
            const std::lock_guard lock(m_mutex);

            m_db[topic] = {.cb = cb};

            SPSP_LOGD("Inserted topic '%s' with callback (renews in %d min)",
                      topic.c_str(), m_db[topic].lifetime);
        }

        /**
         * @brief Removes entry from database
         *
         * @param topic Topic
         */
        void remove(const std::string topic)
        {
            const std::lock_guard lock(m_mutex);

            m_db.erase(topic);

            SPSP_LOGD("Removed topic '%s'", topic.c_str());
        }

        /**
         * @brief Calls callbacks for incoming data
         *
         * @param topic Topic
         * @param payload Data
         */
        void callCb(const std::string topic, const std::string payload)
        {
            m_mutex.lock();

            if (m_db.find(topic) != m_db.end()) {
                auto cb = m_db[topic].cb;

                m_mutex.unlock();

                SPSP_LOGD("Calling user callback for topic '%s'",
                          topic.c_str());

                // Call user's callback
                cb(topic, payload);

                return;
            }

            m_mutex.unlock();

            SPSP_LOGD("No entry (callback) for topic '%s'", topic.c_str());
        }

        /**
         * @brief Time tick callback
         *
         * Decrements subscribe database lifetimes.
         * If any item expires, renews it.
         */
        void tick()
        {
            m_mutex.lock();
            SPSP_LOGD("Tick running");

            for (auto const& [topic, entry] : m_db) {
                m_db[topic].lifetime--;

                // Expired -> renew it
                if (entry.lifetime == 0) {
                    SPSP_LOGD("Topic '%s' expired (renewing)", topic.c_str());

                    m_mutex.unlock();
                    bool extended = m_client->subscribe(topic, entry.cb);
                    m_mutex.lock();

                    if (!extended) {
                        SPSP_LOGE("Topic '%s' can't be extended. Will try again in next tick.",
                                topic.c_str());

                        m_db[topic].lifetime++;
                    }
                }
            }

            m_mutex.unlock();
            SPSP_LOGD("Tick done");
        }
    };
} // namespace SPSP::Nodes

#undef SPSP_LOG_TAG
