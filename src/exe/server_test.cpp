#include <core/network_mock.hpp>
#include <core/api.hpp>
#include <actors/server.hpp>
#include <logic/server_state.hpp>

#include <iostream>
#include <string>

using namespace home_task;

constexpr uint64_t TestId = 1;

int main() {
    auto network = std::make_shared<network_mock::NetworkMock>(2);
    auto serverStateNop = std::make_unique<logic::ServerStateNop>();

    std::unique_ptr<actors::ServerRunner> runner = std::make_unique<actors::ServerRunner>(
            network_mock::NetworkClient(network, magic_numbers::ServerId),
            std::move(serverStateNop));

    network->Send(magic_numbers::ServerId, TestId, network_mock::MakePoisonMessage());

    runner->Run();

    return 0;
}