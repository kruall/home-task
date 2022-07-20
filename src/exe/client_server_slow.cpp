#include "client_server_template.hpp"


using namespace home_task;

int main() {
    exe::Test<logic::ServerState, logic::ClientState, 1, 100>();
    return 0;
}