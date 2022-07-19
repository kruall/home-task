#pragma once

#include <core/api.hpp>

#include <random>


namespace home_task::logic {

struct IClientState {
    virtual ~IClientState() {}

    virtual api::UpdateValueRequest GenerateUpdateValueRequest(std::mt19937 &_gen) = 0;

    virtual api::InsertValueRequest GenerateInsertValueRequest(std::mt19937 &_gen) = 0;

    virtual api::DeleteValueRequest GenerateDeleteValueRequest(std::mt19937 &_gen) = 0;

    virtual void HandleUpdateValueResponse(const api::UpdateValueResponse &_response) = 0;

    virtual void HandleInsertValueResponse(const api::InsertValueResponse &_response) = 0;

    virtual void HandleDeleteValueResponse(const api::DeleteValueResponse &_response) = 0;

    virtual void HandleState(const api::State &_response) = 0;

};

struct ClientStateNop : IClientState {
    virtual ~ClientStateNop(){}

    virtual api::UpdateValueRequest GenerateUpdateValueRequest(std::mt19937 &) {
        log::WriteState("ClientStateNop::GenerateUpdateValueRequest");
        return api::UpdateValueRequest(0, 0);
    }

    virtual api::InsertValueRequest GenerateInsertValueRequest(std::mt19937 &) {
        log::WriteState("ClientStateNop::GenerateInsertValueRequest");
        return api::InsertValueRequest(0, 0);
    }

    virtual api::DeleteValueRequest GenerateDeleteValueRequest(std::mt19937 &) {
        log::WriteState("ClientStateNop::GenerateDeleteValueRequest");
        return api::DeleteValueRequest(0);
    }

    virtual void HandleUpdateValueResponse(const api::UpdateValueResponse &) {
        log::WriteState("ClientStateNop::HandleUpdateValueResponse");
    }

    virtual void HandleInsertValueResponse(const api::InsertValueResponse &) {
        log::WriteState("ClientStateNop::HandleInsertValueResponse");
    }

    virtual void HandleDeleteValueResponse(const api::DeleteValueResponse &) {
        log::Write("ClientStateNop::HandleDeleteValueResponse");
    }

    virtual void HandleState(const api::State &) {
        log::Write("ClientStateNop::HandleState");
    }
};

};