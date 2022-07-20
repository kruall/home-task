#include "server.hpp"

#include <chrono>
#include <thread>
#include <iostream>

using namespace home_task::actors;
using namespace home_task::network_mock;
using namespace home_task::api;



void ServerRunner::UpdateValue(UpdateValueRequest *_request, uint64_t _sender) {
    State_->MoveIterationForClient(_sender, _request->PreviousIteration_);
    auto response = State_->UpdateValue(*_request);
    response.Modificatoins_ = State_->GetNextHistory(_sender);
    response.Iteration_ = State_->GetIteration();
    Client_.Send(_sender, MakeResponseMessage(std::move(response)));
}

void ServerRunner::InsertValue(InsertValueRequest *_request, uint64_t _sender) {
    State_->MoveIterationForClient(_sender, _request->PreviousIteration_);
    auto response = State_->InsertValue(*_request);
    response.Modificatoins_ = State_->GetNextHistory(_sender);
    response.Iteration_ = State_->GetIteration();
    Client_.Send(_sender, MakeResponseMessage(std::move(response)));
}

void ServerRunner::DeleteValue(DeleteValueRequest *_request, uint64_t _sender) {
    State_->MoveIterationForClient(_sender, _request->PreviousIteration_);
    auto response = State_->DeleteValue(*_request);
    response.Modificatoins_ = State_->GetNextHistory(_sender);
    response.Iteration_ = State_->GetIteration();
    Client_.Send(_sender, MakeResponseMessage(std::move(response)));
}

void ServerRunner::LoadState(uint64_t _sender) {
    auto state = State_->LoadState();
    state.Iteration_ = State_->GetIteration();
    Client_.Send(_sender, MakeResponseMessage(std::move(state)));
}

void ServerRunner::Sync(SyncRequest *_request, uint64_t _sender) {
    State_->MoveIterationForClient(_sender, _request->PreviousIteration_);
    api::SyncResponse response;
    response.Modificatoins_ = State_->GetNextHistory(_sender);
    response.Iteration_ = State_->GetIteration();
    Client_.Send(_sender, MakeResponseMessage(std::move(response)));
}

void ServerRunner::Run() {
    log::WriteServerRunner("ServerRunner::Run");
    for (;;) {
        auto startReceiving = std::chrono::steady_clock::now();
        auto message = Client_.ReceiveWithWaiting();
        std::chrono::duration<double> durationOfReceiving = std::chrono::steady_clock::now() - startReceiving;
        log::WriteServerRunner("ServerRunner::Run{Wait message ", durationOfReceiving.count(), "s}");
        if (message->Type_ == static_cast<uint32_t>(EMessageType::Poison)) {
            log::WriteServerRunner("ServerRunner::Run{Poisoned}");
            break;
        }

        auto startProcessing = std::chrono::steady_clock::now();
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
        case (uint32_t)EAPIEventsType::SyncRequest:
            Sync(std::any_cast<SyncRequest>(&message->Record_), message->Sender_);
            break;
        case (uint32_t)EAPIEventsType::LoadState:
            LoadState(message->Sender_);
            break;
        }
        std::chrono::duration<double> durationOfProcessing = std::chrono::steady_clock::now() - startProcessing;
        log::WriteServerRunner("ServerRunner::Run{Processing message ", durationOfReceiving.count(), "s}");
        State_->CutHistory();
    }
}
