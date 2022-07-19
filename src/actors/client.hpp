#pragma once

#include <core/network_mock.hpp>
#include <core/api.hpp>
#include <core/log.hpp>
#include <logic/client_state.hpp>

#include <memory>

namespace home_task::actors {

struct ClientRunner {
    network_mock::NetworkClient Client_;
    std::unique_ptr<logic::IClientState> State_;

    ClientRunner(network_mock::NetworkClient &&_client, std::unique_ptr<logic::IClientState> &&_state)
        : Client_(std::move(_client))
        , State_(std::move(_state))
    {}

    ~ClientRunner() {
        log::WriteDestructor("~ClientRunner");
    }

    void Run();
};

}
