#include "client.hpp"

#include <random>
#include <thread>
#include <chrono>

using namespace home_task::actors;
using namespace home_task::network_mock;
using namespace home_task::api;


void ClientRunner::Run() {
    log::Write("ClientRunner::Run");

    std::random_device rd;
    std::mt19937 gen(rd());
    // 0 - LoadState | 1 - UpdateValue | 2 - InsertValue | 3 - DeleteValue | 4 - Sync
    std::uniform_int_distribution<> commandDstrib(0, 3);

    enum {
        LOAD_STATE = 0,
        UPDATE_VALUE,
        INSERT_VALUE,
        DELETE_VALUE,
        SYNC
    };

    for (uint32_t iteration = 0;; ++iteration) {

        auto type = iteration ? commandDstrib(gen) : 0;
        switch (type) {
        case LOAD_STATE:
            Client_.Send(magic_numbers::ServerId, MakeLoadStateMessage());
            break;
        case UPDATE_VALUE:
            Client_.Send(magic_numbers::ServerId, MakeRequestMessage(State_->GenerateUpdateValueRequest(gen)));
            break;
        case INSERT_VALUE:
            Client_.Send(magic_numbers::ServerId, MakeRequestMessage(State_->GenerateInsertValueRequest(gen)));
            break;
        case DELETE_VALUE:
            Client_.Send(magic_numbers::ServerId, MakeRequestMessage(State_->GenerateDeleteValueRequest(gen)));
            break;
        }

        auto messageOpt = Client_.Receive();
        if (!messageOpt) {
            log::Write("ClientRunner::Run{WaitMessage}");
            Client_.WaitMessage();
            continue;
        }
        auto &message = *messageOpt;
        if (message->Type_ == static_cast<uint32_t>(EMessageType::Poison)) {
            log::Write("ClientRunner::Run{Poisoned}");
            break;
        }

        switch (message->Type_) {
        case (uint32_t)EAPIEventsType::UpdateValueResponse:
            State_->HandleUpdateValueResponse(*std::any_cast<UpdateValueResponse>(&message->Record_));
            break;
        case (uint32_t)EAPIEventsType::InsertValueResponse:
            State_->HandleInsertValueResponse(*std::any_cast<InsertValueResponse>(&message->Record_));
            break;
        case (uint32_t)EAPIEventsType::DeleteValueResponse:
            State_->HandleDeleteValueResponse(*std::any_cast<DeleteValueResponse>(&message->Record_));
            break;
        case (uint32_t)EAPIEventsType::State:
            State_->HandleState(*std::any_cast<State>(&message->Record_));
            break;
        }

        log::Write("ClientRunner::Run{Sleep 200ms}");
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(200ms);
    }
}