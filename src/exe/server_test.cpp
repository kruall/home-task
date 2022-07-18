#include <core/network_mock.hpp>
#include <core/api.hpp>
#include <actors/server.hpp>

#include <iostream>
#include <string>

using namespace home_task;

constexpr uint64_t TestId = 1;

int main() {
    auto network = std::make_shared<network_mock::NetworkMock>(2);
    std::unique_ptr<actors::ServerRunner> runner = std::make_unique<actors::ServerRunner>(
            network, magic_numbers::ServerId);

    network->Send(magic_numbers::ServerId, TestId, api::MakeLoadStateMessage());
    network->Send(magic_numbers::ServerId, TestId, network_mock::MakePoisonMessage());

    runner->Run();

    return 0;
}