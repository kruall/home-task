#pragma once

#include <core/network_mock.hpp>
#include <core/api.hpp>
#include <core/log.hpp>
#include <logic/server_state.hpp>

#include <memory>


namespace home_task::actors {

struct ServerRunner {
    network_mock::NetworkClient Client_;
    std::unique_ptr<logic::IServerState> State_;

    ServerRunner(network_mock::NetworkClient &&_client, std::unique_ptr<logic::IServerState> &&_state)
        : Client_(std::move(_client))
        , State_(std::move(_state))
    {}

    ~ServerRunner() {
        log::WriteDestructor("~ServerRunner");
    }

    void UpdateValue(api::UpdateValueRequest *_request, uint64_t _sender);
    void InsertValue(api::InsertValueRequest *_request, uint64_t _sender);
    void DeleteValue(api::DeleteValueRequest *_request, uint64_t _sender);
    void LoadState(uint64_t _sender);
    void Run();
};

}