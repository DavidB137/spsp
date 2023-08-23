/**
 * @file spsp_mqtt.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief MQTT far layer for SPSP
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <future>

#include "spsp_layers.hpp"
#include "spsp_mqtt_adapter.hpp"
#include "spsp_mqtt_types.hpp"
#include "spsp_node.hpp"

namespace SPSP::FarLayers::MQTT
{
    /**
     * @brief MQTT far layer
     *
     */
    class MQTT : public IFarLayer
    {
        Config m_conf;                           //!< Configuration
        Adapter m_adapter;                       //!< Platform-specific MQTT adapter
        bool m_initializing = true;              //!< Whether we are currently in initializing phase
        std::promise<void> m_connectingPromise;  //!< Promise to block until successful connection is made

    public:
        /**
         * @brief Constructs a new MQTT layer object
         *
         * Block until connection is successfully made.
         *
         * @param conf Configuration
         * @throw AdapterError when adapter can't be constructed
         * @throw ConnectionError when connection can't be established
         */
        MQTT(const Config& conf);

        /**
         * @brief Destroys MQTT layer object
         *
         */
        ~MQTT();

        /**
         * @brief Publishes message coming from node
         *
         * @param src Source address
         * @param topic Topic
         * @param payload Payload (data)
         * @return true Delivery successful
         * @return false Delivery failed
         */
        bool publish(const std::string& src, const std::string& topic,
                     const std::string& payload);

        /**
         * @brief Subscribes to given topic
         *
         * Should be used by `INode` only!
         *
         * @param topic Topic
         * @return true Subscribe successful
         * @return false Subscribe failed
         */
        bool subscribe(const std::string& topic);

        /**
         * @brief Unsubscribes from given topic
         *
         * Should be used by `INode` only!
         *
         * @param topic Topic
         * @return true Unsubscribe successful
         * @return false Unsubscribe failed
         */
        bool unsubscribe(const std::string& topic);

    protected:
        /**
         * @brief Signalizes successful initial connection to broker
         *
         */
        void connectedCb();

        /**
         * @brief Callback for underlaying adapter to receive subscribe data
         *
         * @param topic Topic
         * @param payload Payload
         */
        void subDataCb(const std::string& topic, const std::string& payload);
    };
} // namespace SPSP::FarLayers::MQTT
