#pragma once

#include <core/api.hpp>


namespace home_task::logic {

struct IServerState {
    virtual ~IServerState() {}

    virtual api::UpdateValueResponse UpdateValue(const api::UpdateValueRequest &_request) = 0;

    virtual api::InsertValueResponse InsertValue(const api::InsertValueRequest &_request) = 0;

    virtual api::DeleteValueResponse DeleteValue(const api::DeleteValueRequest &_request) = 0;

    virtual api::State LoadState() = 0;

};

struct ServerStateNop : IServerState {
    virtual ~ServerStateNop(){}

    api::UpdateValueResponse UpdateValue(const api::UpdateValueRequest &) override {
        log::WriteState("ServerStateNop::UpdateValue");
        return api::UpdateValueResponse();
    }

    virtual api::InsertValueResponse InsertValue(const api::InsertValueRequest &) {
        log::WriteState("ServerStateNop::InsertValue");
        return api::InsertValueResponse(0, {});
    }

    virtual api::DeleteValueResponse DeleteValue(const api::DeleteValueRequest &) {
        log::WriteState("ServerStateNop::DeleteValue");
        return api::DeleteValueResponse();
    }

    virtual api::State LoadState() {
        log::WriteState("ServerStateNop::LoadState");
        return api::State({});
    }
};

};