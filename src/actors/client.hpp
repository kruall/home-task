#pragma once

#include <core/network_mock.hpp>
#include <core/api.hpp>
#include <core/log.hpp>
#include <logic/client_state.hpp>

#include <memory>
#include <chrono>
#include <iomanip>

namespace home_task::actors {

struct ClientRunner {
    network_mock::NetworkClient Client_;
    std::unique_ptr<logic::IClientState> State_;
    uint32_t Id_;

    std::chrono::duration<double> WorkTime_;
    uint64_t IteratoinCount_ = 0;
    uint64_t SentBytes_ = 0;
    uint64_t ReceivedBytes_ = 0;

    ClientRunner(network_mock::NetworkClient &&_client, std::unique_ptr<logic::IClientState> &&_state, uint32_t _id)
        : Client_(std::move(_client))
        , State_(std::move(_state))
        , Id_(_id)
    {}

    ~ClientRunner() {
        log::WriteDestructor("~ClientRunner");
    }

    void Run();

    std::string PrettyMemory(double _mem) const {
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

    std::string PrintStat() const {
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
};

}
