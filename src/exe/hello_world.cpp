#include <core/network_mock.hpp>
#include <iostream>
#include <string>

using namespace home_task;

int main() {
    network_mock::NetworkMock network(1);
    auto msg = network_mock::MakeStringMessage("Hello world!");
    network.Send(magic_numbers::ServerId, magic_numbers::ServerId, std::move(msg));
    auto receivedMsg = network.Receive(magic_numbers::ServerId);
    if (!receivedMsg) {
        std::cerr << "ERROR: msg wasn't delivered!\n";
        return 1;
    }
    std::string* s = std::any_cast<std::string>(&receivedMsg.value()->Record_);
    if (!s) {
        std::cerr << "ERROR: record wasn't string!\n";
        return 1;
    }
    std::cout << *s << std::endl;
    return 0;
}