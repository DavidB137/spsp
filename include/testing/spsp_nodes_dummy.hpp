/**
 * @file spsp_nodes_dummy.hpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief Dummy nodes for testing
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include "spsp_node.hpp"

namespace SPSP::Nodes
{
    /**
     * @brief Dummy local node for testing
     *
     * @tparam TLocalLayer Type of local layer
     */
    template <typename TLocalLayer>
    class DummyLocalNode : public ILocalNode<TLocalLayer>
    {
    public:
        using LocalAddrT = typename TLocalLayer::LocalAddrT;
        using LocalMessageT = typename TLocalLayer::LocalMessageT;

        using ILocalNode<TLocalLayer>::ILocalNode;

        virtual bool publish(const std::string& topic,
                             const std::string& payload) { return true; }

        virtual bool subscribe(const std::string& topic, SubscribeCb cb) { return true; }

        virtual bool unsubscribe(const std::string& topic) { return true; }

    protected:
        virtual bool processProbeReq(const LocalMessageT& req,
                                     int rssi = NODE_RSSI_UNKNOWN) { return true; }

        virtual bool processProbeRes(const LocalMessageT& req,
                                     int rssi = NODE_RSSI_UNKNOWN) { return true; }

        virtual bool processPub(const LocalMessageT& req,
                                int rssi = NODE_RSSI_UNKNOWN) { return true; }

        virtual bool processSubReq(const LocalMessageT& req,
                                   int rssi = NODE_RSSI_UNKNOWN) { return true; }

        virtual bool processSubData(const LocalMessageT& req,
                                    int rssi = NODE_RSSI_UNKNOWN) { return true; }

        virtual bool processUnsub(const LocalMessageT& req,
                                  int rssi = NODE_RSSI_UNKNOWN) { return true; }
    };
} // namespace SPSP::Nodes
