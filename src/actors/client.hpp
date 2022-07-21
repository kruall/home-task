#pragma once

#include <core/network_mock.hpp>
#include <core/api.hpp>
#include <core/log.hpp>
#include <logic/client_state.hpp>

#include <memory>
#include <chrono>
#include <iomanip>

namespace home_task::actors {

struct ClientRunner {
    network_mock::NetworkClient Client_;
    std::unique_ptr<logic::IClientState> State_;
    model::ClientId Id_;

    std::chrono::duration<double> WorkTime_;
    uint64_t IteratoinCount_ = 0;
    uint64_t SentBytes_ = 0;
    uint64_t ReceivedBytes_ = 0;

    ClientRunner(network_mock::NetworkClient &&_client, std::unique_ptr<logic::IClientState> &&_state, model::ClientId _id)
        : Client_(std::move(_client))
        , State_(std::move(_state))
        , Id_(_id)
    {}

    ~ClientRunner() {
        log::WriteDestructor("~ClientRunner");
    }

    void Run();
    std::string PrettyMemory(double _mem) const;
    std::string PrintStat() const;
};

}
