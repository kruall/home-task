#include <core/network_mock.hpp>
#include <iostream>
#include <string>



int main() {
    home_task::network_mock::NetworkMock network(1);
    auto msg = home_task::network_mock::MakeStringMessage("Hello world!");
    network.Send(0, std::move(msg));
    auto receivedMsg = network.Receive(0);
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