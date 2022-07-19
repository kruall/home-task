#include "client.hpp"

#include <random>
#include <thread>
#include <chrono>

using namespace home_task::actors;
using namespace home_task::network_mock;
using namespace home_task::api;


void ClientRunner::Run() {
    log::WriteClientRunner("ClientRunner::Run");

    std::random_device rd;
    std::mt19937 gen(rd());
    enum {
        LOAD_STATE = 0,
        UPDATE_VALUE = 1,
        INSERT_VALUE = 2,
        DELETE_VALUE = 3,
        SYNC = 4,
    };

    auto leftRangeBorder = magic_numbers::UserSendLoadState ? LOAD_STATE : UPDATE_VALUE;
    auto rightRangeBorder = magic_numbers::UserSendSync ? SYNC : DELETE_VALUE;

    std::uniform_int_distribution<> commandDstrib(leftRangeBorder, rightRangeBorder);

    Client_.Send(magic_numbers::ServerId, MakeConnectMessage());

    auto start = std::chrono::steady_clock::now();
    for (IteratoinCount_ = 0;; ++IteratoinCount_) {
        log::WriteClientRunner("ClientRunner::Run{iteration=", IteratoinCount_, ", start}");
        auto type = IteratoinCount_ ? commandDstrib(gen) : 0;
        std::unique_ptr<MessageRecord> msg;
        switch (type) {
        case LOAD_STATE:
            msg = MakeLoadStateMessage();
            break;
        case UPDATE_VALUE:
            msg = MakeRequestMessage(State_->GenerateUpdateValueRequest(gen));
            break;
        case INSERT_VALUE:
            msg = MakeRequestMessage(State_->GenerateInsertValueRequest(gen));
            break;
        case DELETE_VALUE:
            msg = MakeRequestMessage(State_->GenerateDeleteValueRequest(gen));
            break;
        case SYNC:
            msg = MakeRequestMessage(State_->GenerateSyncRequest());
            break;
        }
        SentBytes_ += msg->Size_;
        Client_.Send(magic_numbers::ServerId, std::move(msg));

        auto startReceiving = std::chrono::steady_clock::now();
        auto message = Client_.ReceiveWithWaiting();
        ReceivedBytes_ += message->Size_;
        std::chrono::duration<double> durationOfReceiving = std::chrono::steady_clock::now() - startReceiving;
        log::WriteClientRunner("ClientRunner::Run{Wait response ", durationOfReceiving.count(), "s}");
        if (message->Type_ == static_cast<uint32_t>(EMessageType::Poison)) {
            log::WriteClientRunner("ClientRunner::Run{id=", Id_, ", iteration=", IteratoinCount_, ", Poisoned}");
            break;
        }

        log::WriteClientRunner("ClientRunner::Run{iteration=", IteratoinCount_, ", handling}");
        auto startHandling = std::chrono::steady_clock::now();
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
        case (uint32_t)EAPIEventsType::SyncResponse:
            State_->HandleSyncResponse(*std::any_cast<SyncResponse>(&message->Record_));
            break;
        case (uint32_t)EAPIEventsType::State:
            State_->HandleState(*std::any_cast<State>(&message->Record_));
            break;
        }
        std::chrono::duration<double> durationOfHandling = std::chrono::steady_clock::now() - startHandling;
        log::WriteClientRunner("ClientRunner::Run{id=", Id_, ", iteration=", IteratoinCount_, ", duratoin of handling message ", durationOfHandling.count(), "s}");
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(200ms);
    }
    WorkTime_ = std::chrono::steady_clock::now() - start;
}

std::string ClientRunner::PrettyMemory(double _mem) const {
    std::ostringstream sout;
    sout << std::fixed << std::setprecision(2);
    if (_mem > 1e9) {
        sout << _mem / 1e9 << "GB";
    } else if (_mem > 1e6) {
        sout << _mem / 1e6 << "MB";
    } else if (_mem > 1e3) {
        sout << _mem / 1e3 << "kB";
    } else {
        sout << _mem << "B";
    }
    return sout.str();
}

std::string ClientRunner::PrintStat() const {
    std::ostringstream sout;
    sout << std::fixed << std::setprecision(2);
    sout << "Id# "  << Id_
        << " SentBytes# " << PrettyMemory(SentBytes_)
        << " ReceivedBytes# " << PrettyMemory(ReceivedBytes_) << std::endl;
    sout << " SBPS# " << PrettyMemory(SentBytes_ / WorkTime_.count()) << "/s"
        << " RBPS# " << PrettyMemory(ReceivedBytes_ / WorkTime_.count()) << "/s" << std::endl;
    sout << " Iterations# " << IteratoinCount_
        << " IterationsPerSecond# " << IteratoinCount_ / WorkTime_.count()
        << " WorkTime# " << WorkTime_.count() << 's' << std::endl;
    return sout.str();
}
