#include "server.hpp"

#include <chrono>
#include <thread>
#include <iostream>

using namespace home_task::actors;
using namespace home_task::network_mock;
using namespace home_task::api;



void ServerRunner::UpdateValue(UpdateValueRequest *_request, uint64_t _sender) {
    log::Write("UpdateValue");
    Client_.Send(_sender, MakeResponseMessage<UpdateValueResponse>(State_->UpdateValue(*_request)));
}

void ServerRunner::InsertValue(InsertValueRequest *_request, uint64_t _sender) {
    log::Write("InsertValue");
    Client_.Send(_sender, MakeResponseMessage<InsertValueResponse>(State_->InsertValue(*_request)));
}

void ServerRunner::DeleteValue(DeleteValueRequest *_request, uint64_t _sender) {
    log::Write("DeleteValue");
    Client_.Send(_sender, MakeResponseMessage<DeleteValueResponse>(State_->DeleteValue(*_request)));
}

void ServerRunner::LoadState(uint64_t _sender) {
    log::Write("LoadState");
    Client_.Send(_sender, MakeResponseMessage<State>(State_->LoadState()));
}

void ServerRunner::Run() {
    log::Write("ServerRunner::Run");
    for (;;) {
        auto messageOpt = Client_.Receive();
        if (!messageOpt) {
            log::Write("ServerRunner::Run{WaitMessage}");
            Client_.WaitMessage();
            continue;
        }
        auto &message = *messageOpt;
        if (message->Type_ == static_cast<uint32_t>(EMessageType::Poison)) {
            log::Write("ServerRunner::Run{Poisoned}");
            break;
        }
        switch (message->Type_) {
        case (uint32_t)EAPIEventsType::UpdateValueRequest:
            UpdateValue(std::any_cast<UpdateValueRequest>(&message->Record_), message->Sender_);
            break;
        case (uint32_t)EAPIEventsType::InsertValueRequest:
            InsertValue(std::any_cast<InsertValueRequest>(&message->Record_), message->Sender_);
            break;
        case (uint32_t)EAPIEventsType::DeleteValueRequest:
            DeleteValue(std::any_cast<DeleteValueRequest>(&message->Record_), message->Sender_);
            break;
        case (uint32_t)EAPIEventsType::LoadState:
            LoadState(message->Sender_);
            break;
        }
    }
}
