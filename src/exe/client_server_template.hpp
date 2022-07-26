#pragma once

#include <core/network_mock.hpp>
#include <actors/server.hpp>
#include <logic/server_state.hpp>
#include <actors/client.hpp>
#include <logic/client_state.hpp>

#include <iostream>
#include <string>
#include <thread>


namespace home_task::exe {

template <typename _ServerStateType, typename _ClientStateType, uint32_t _ClientCount, uint64_t _CellCount>
void Test() {
    constexpr uint32_t clientCount = _ClientCount;
    constexpr uint32_t mainThreadId = clientCount + 1;
    auto network = std::make_shared<network_mock::NetworkMock>(clientCount + 2);

    network_mock::NetworkClient networkClient(network, mainThreadId);

    constexpr uint64_t cellCount = _CellCount;
    std::vector<model::Cell> initCells;
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> valueDistrib(0);
        initCells.reserve(cellCount);
        for (uint64_t idx = 0; idx < cellCount; ++idx) {
            initCells.emplace_back(idx +  1, valueDistrib(gen));
        }
    }

    auto serverState = std::make_unique<_ServerStateType>(initCells);
    std::unique_ptr<actors::ServerRunner> serverRunner = std::make_unique<actors::ServerRunner>(
            network_mock::NetworkClient(network, magic_numbers::ServerId),
            std::move(serverState));

    std::vector<std::unique_ptr<actors::ClientRunner>> clientsRunners;
    clientsRunners.reserve(clientCount);
    for (uint32_t idx = 0; idx < clientCount; ++idx) {
        auto clientState = std::make_unique<_ClientStateType>();
        clientsRunners.emplace_back(std::make_unique<actors::ClientRunner>(
                network_mock::NetworkClient(network, idx + 1),
                std::move(clientState),
                idx + 1));
    }

    std::thread serverThread(&actors::ServerRunner::Run, serverRunner.get());
    std::vector<std::thread> clientsThreads;
    clientsThreads.reserve(clientCount);
    for (uint32_t idx = 0; idx < clientCount; ++idx) {
        clientsThreads.emplace_back(&actors::ClientRunner::Run, clientsRunners[idx].get());
    }

    log::Write("Main thread goes to sleep for 100s");
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(100s);
    log::Write("Main thread woke up");

    networkClient.Send(magic_numbers::ServerId, network_mock::MakePoisonMessage());
    for (uint32_t idx = 0; idx < clientCount; ++idx) {
        networkClient.Send(idx + 1, network_mock::MakePoisonMessage());
    }

    serverThread.join();
    for (auto &thr : clientsThreads) {
        thr.join();
    }

    for (auto &runner : clientsRunners) {
        std::cout << runner->PrintStat();
    }
}

}
