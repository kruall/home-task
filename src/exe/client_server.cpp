#include "client_server_template.hpp"


using namespace home_task;

int main() {
    exe::Test<logic::ServerState, logic::FastSmallClientState, 20, 10'000'000>();
    return 0;
}