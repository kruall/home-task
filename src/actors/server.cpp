#include "server.hpp"

#include <chrono>
#include <thread>
#include <iostream>

using namespace home_task::actors;
using namespace home_task::network_mock;
using namespace home_task::api;



void ServerRunner::UpdateValue(UpdateValueRequest *_request, uint64_t _sender) {

}

void ServerRunner::InsertValue(InsertValueRequest *_request, uint64_t _sender) {

}

void ServerRunner::DeleteValue(DeleteValueRequest *_request, uint64_t _sender) {

}

void ServerRunner::LoadState(uint64_t _sender) {

}

void ServerRunner::Run() {
    for (;;) {
        auto messageOpt = Network_->Receive(MailBox_);
        if (!messageOpt) {
            Network_->WaitMessage(MailBox_);
            continue;
        }
        auto &message = *messageOpt;
        if (message->Type_ == static_cast<uint32_t>(EMessageType::Poison)) {
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
