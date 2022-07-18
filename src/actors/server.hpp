#pragma once

#include <core/network_mock.hpp>
#include <core/api.hpp>
#include <core/log.hpp>

#include <memory>


namespace home_task::actors {

struct ServerRunner {
    std::shared_ptr<network_mock::NetworkMock> Network_;
    uint32_t MailBox_;

    ~ServerRunner() {
        log::Write("~ServerRunner");
    }

    ServerRunner(const std::shared_ptr<network_mock::NetworkMock> &_network, uint32_t _mailBox)
        : Network_(_network)
        , MailBox_(_mailBox)
    {}

    void UpdateValue(api::UpdateValueRequest *_request, uint64_t _sender);
    void InsertValue(api::InsertValueRequest *_request, uint64_t _sender);
    void DeleteValue(api::DeleteValueRequest *_request, uint64_t _sender);
    void LoadState(uint64_t _sender);
    void Run();
};

}