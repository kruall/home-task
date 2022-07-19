#include <core/network_mock.hpp>
#include <actors/server.hpp>
#include <logic/server_state.hpp>
#include <actors/client.hpp>
#include <logic/client_state.hpp>

#include <iostream>
#include <string>
#include <thread>

using namespace home_task;

int main() {
    constexpr uint32_t clientCount = 20;
    constexpr uint32_t mainThreadId = clientCount + 1;
    auto network = std::make_shared<network_mock::NetworkMock>(clientCount + 2);

    network_mock::NetworkClient networkClient(network, mainThreadId);

    auto serverStateNop = std::make_unique<logic::ServerStateNop>();
    std::unique_ptr<actors::ServerRunner> serverRunner = std::make_unique<actors::ServerRunner>(
            network_mock::NetworkClient(network, magic_numbers::ServerId),
            std::move(serverStateNop));

    std::vector<std::unique_ptr<actors::ClientRunner>> clientsRunners;
    clientsRunners.reserve(clientCount);
    for (uint32_t idx = 0; idx < clientCount; ++idx) {
        auto clientStateNop = std::make_unique<logic::ClientStateNop>();
        clientsRunners.emplace_back(std::make_unique<actors::ClientRunner>(
                network_mock::NetworkClient(network, idx + 1),
                std::move(clientStateNop)));
    }

    std::thread serverThread(&actors::ServerRunner::Run, serverRunner.get());
    std::vector<std::thread> clientsThreads;
    clientsThreads.reserve(clientCount);
    for (uint32_t idx = 0; idx < clientCount; ++idx) {
        clientsThreads.emplace_back(&actors::ClientRunner::Run, clientsRunners[idx].get());
    }

    log::Write("Main thread goes to sleep for 5s");
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(5s);
    log::Write("Main thread woke up");

    networkClient.Send(magic_numbers::ServerId, network_mock::MakePoisonMessage());
    for (uint32_t idx = 0; idx < clientCount; ++idx) {
        networkClient.Send(idx + 1, network_mock::MakePoisonMessage());
    }

    serverThread.join();
    for (auto &thr : clientsThreads) {
        thr.join();
    }

    return 0;
}