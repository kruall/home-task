#pragma once

#include <core/api.hpp>

#include <list>
#include <unordered_map>
#include <unordered_set>
#include <queue>


namespace home_task::logic {

struct IServerState {
    virtual ~IServerState() {}

    virtual api::UpdateValueResponse UpdateValue(const api::UpdateValueRequest &_request) = 0;

    virtual api::InsertValueResponse InsertValue(const api::InsertValueRequest &_request) = 0;

    virtual api::DeleteValueResponse DeleteValue(const api::DeleteValueRequest &_request) = 0;

    virtual api::State LoadState() = 0;

    virtual void Connect(model::ClientId _id) = 0;

    virtual void GetNextHistory(model::ClientId _id, api::GenericResponse *_response) = 0;

    virtual model::IterationI GetIteration() = 0;

    virtual void MoveIterationForClient(model::ClientId _id, model::IterationId _iteration) = 0;

    virtual void CutHistory() = 0;
};

struct ServerStateNop : IServerState {
    virtual ~ServerStateNop(){}

    ServerStateNop() = default;
    ServerStateNop(const std::vector<model::Cell> &)
    {}

    api::UpdateValueResponse UpdateValue(const api::UpdateValueRequest &) override {
        log::WriteServerState("ServerStateNop::UpdateValue");
        return api::UpdateValueResponse();
    }

    api::InsertValueResponse InsertValue(const api::InsertValueRequest &) override {
        log::WriteServerState("ServerStateNop::InsertValue");
        return api::InsertValueResponse(0);
    }

    api::DeleteValueResponse DeleteValue(const api::DeleteValueRequest &) override {
        log::WriteServerState("ServerStateNop::DeleteValue");
        return api::DeleteValueResponse();
    }

    api::State LoadState() override {
        log::WriteServerState("ServerStateNop::LoadState");
        return api::State({});
    }

    void Connect(model::ClientId _id) override {
        log::WriteServerState("ServerStateNop::LoadState");
    }

    void GetNextHistory(model::ClientId, api::GenericResponse *) override {
        log::WriteServerState("ServerStateNop::GetNextHistory");
        return;
    }
    
    model::IterationI GetIteration() override {
        return 0;
    }

    void MoveIterationForClient(model::ClientId, model::IterationI) override {
        log::WriteServerState("ServerStateNop::MoveIterationForClient");
        return;
    }

    void CutHistory() override {

    }
};

template <typename _Derived>
struct Referable {
    uint32_t ReferenceCount_ = 0;

    void Ref() {
        ReferenceCount_++;
    }

    void Unref();
};

template <typename _Derived>
struct List {
    _Derived *Prev_ = nullptr;
    _Derived *Next_ = nullptr;

    void PutAfter(_Derived *_node);
    void PullOut();
};

template <typename _Derived>
struct DeletableObject {
    _Derived *NearLive_ = nullptr;
    bool Deleted_ = false;
    uint32_t NearRefCount_ = 0;

    void SetNear(_Derived *_near);
    _Derived* FindNear();
};


struct CellState
    : model::Cell
    , Referable<CellState>
    , List<CellState>
    , DeletableObject<CellState>
{
    std::queue<model::CellId> *QueueToRemove_;

    CellState(model::Cell _cell, std::queue<model::CellId> *_queue)
        : model::Cell(_cell)
        , QueueToRemove_(_queue)
    {}
};

template <typename _Derived>
void Referable<_Derived>::Unref() {
    if (--ReferenceCount_ == 1) {
        auto self = static_cast<_Derived*>(this);
        self->QueueToRemove_->push(self->CellId_);
    }
}

template <typename _Derived>
void List<_Derived>::PutAfter(_Derived *_node) {
    if (_node) {
        if (Next_) {
            Next_->Prev_ = _node;
        }
        _node->Next_ = std::exchange(Next_, _node);
        _node->Prev_ = static_cast<_Derived*>(this);
        _node->Ref();
    }
}

template <typename _Derived>
void List<_Derived>::PullOut() {
    if (Prev_) {
        Prev_->Next_ = Next_;
        static_cast<_Derived*>(this)->Unref();
    }
    if (Next_) {
        Next_->Prev_ = Prev_;
    }
    Prev_ = Next_ = nullptr;
}

template <typename _Derived>
void DeletableObject<_Derived>::SetNear(_Derived *_near) {
    if (NearLive_) {
        NearLive_->Unref();
        NearLive_->NearRefCount_--;
    }
    if (_near) {
        NearLive_ = _near;
        NearLive_->Ref();
        NearLive_->NearRefCount_++;
    }
}

template <typename _Derived>
_Derived* DeletableObject<_Derived>::FindNear() {
    std::vector<_Derived*> seen;
    if (NearLive_->Deleted_) {
        while (NearLive_->Deleted_) {
            NearLive_->Unref();
            seen.push_back(NearLive_);
            NearLive_ = NearLive_->NearLive_;
        }
        NearLive_->Ref();
        for (_Derived *node : seen) {
            node->SetNear(NearLive_);
        }
    }
    return NearLive_;
}



using CellStateDict = std::unordered_map<model::CellId, std::unique_ptr<CellState>>;

struct ServerState : IServerState {
    std::queue<model::CellId> QueueToRemove_;
    CellState Root_;
    CellStateDict States_;

    model::OperationDeq History_;
    model::IterationId LastCutIteration_ = 0;
    model::IterationId Iteration_ = 0;


    std::unordered_map<model::ClientId, model::IterationI> LastSeenClients_;
    std::priority_queue<
            std::pair<model::IterationI, model::ClientId>,
            std::vector<std::pair<model::IterationI, model::ClientId>>,
            std::greater<std::pair<model::IterationI, model::ClientId>>>
        OrderedSeenClients_;


    model::CellId NextCellId_ = 1;
    
    ServerState(const model::CellVector &_cells)
        : Root_(model::Cell(0, 0), &QueueToRemove_)
    {
        log::WriteServerState("ServerState{_cells.size()=", _cells.size(), '}');

        CellState *current = &Root_;
        Root_.ReferenceCount_ = 2;

        States_.emplace(0, current);
        CellState *prev = nullptr;
        log::WriteServerState("ServerState{Start}");
        uint32_t idx = 0;
        for (const model::Cell &cell : _cells) {
            log::WriteInit("ServerState{idx}", idx++);
            CellState *node = new CellState(cell, &QueueToRemove_);
            current->PutAfter(node);
            if (current->Next_ != node) {
                log::ForceWrite("ERROR  PutAfter doesn't work");
                std::exit(1);
            }
            node->SetNear(current);
            prev = std::exchange(current, node);
            States_.emplace(cell.CellId_, node);
            NextCellId_ = std::max(NextCellId_, cell.CellId_ + 1);
        }
        current->Next_ =  &Root_;
        Root_.Prev_ = current;
        Root_.SetNear(current);
    }

    virtual ~ServerState(){
        States_[0].release();
    }

    CellState& GetState(model::CellId _cellId) {
        auto it = States_.find(_cellId);
        if (it == States_.end()) {
            log::ForceWrite("failed to find state for cellId# ", _cellId);
            std::exit(1);
        }
        return *(it->second);
    }

    api::UpdateValueResponse UpdateValue(const api::UpdateValueRequest &_request) override {
        log::WriteServerState("ServerState::UpdateValue ", _request.CellId_, ' ',_request.Value_);

        CellState &state = GetState(_request.CellId_);
        if (!state.Deleted_) {
            Iteration_++;;
            state.Value_ = _request.Value_;
            state.Ref();
            History_.emplace_back(model::UpdateValue{.Cell_ = state});
        }
        return api::UpdateValueResponse();
    }

    void CleanQueue() {
        log::WriteServerState("ServerState::CleanQueue ", QueueToRemove_.size());
        uint32_t count = 0;
        while (QueueToRemove_.size()) {
            model::CellId id =  QueueToRemove_.front();
            std::unique_ptr<CellState> &state = States_[id];
            QueueToRemove_.pop();
            if (state->ReferenceCount_ > 1) {
                continue;
            }
            log::WriteServerState("ServerState::CleanQueue{Clean} ", state->CellId_);
            state->SetNear(nullptr);
            state->PullOut();
            States_.erase(state->CellId_);
            count++;
        }
        log::WriteServerState("ServerState::CleanQueue{Cleaned} ", count);
    }

    api::InsertValueResponse InsertValue(const api::InsertValueRequest &_request) override {
        Iteration_++;
        model::CellId cellId = NextCellId_++;

        log::WriteServerState("ServerState::InsertValue ", _request.NearCellId_, ' ', cellId);

        CellState *newState = new CellState(model::Cell(cellId, _request.Value_), &QueueToRemove_);
        CellState *nearState = &GetState(_request.NearCellId_);
        newState->SetNear(nearState);
        nearState->PutAfter(newState);

        CellState *current = newState->Next_;
        CellState *prev = newState;
        while (current && current->NearLive_ != prev) {
            current->SetNear(newState);
            prev = std::exchange(current, current->Next_);
        }

        States_.emplace(cellId, newState);
        newState->Ref();
        History_.emplace_back(model::InsertValue{.NearCellId_ = newState->FindNear()->CellId_, .Cell_ = *newState});
        return api::InsertValueResponse(cellId);
    }

    api::DeleteValueResponse DeleteValue(const api::DeleteValueRequest &_request) override {
        log::WriteServerState("ServerState::DeleteValue ", _request.CellId_);
        CellState &state = GetState(_request.CellId_);

        if (state.Deleted_) {
            return api::DeleteValueResponse();
        }
        Iteration_++;
        state.Deleted_ = true;
        state.Ref();
        History_.emplace_back(model::DeleteValue{.CellId_ = _request.CellId_});
        return api::DeleteValueResponse();
    }

    api::State LoadState() override {
        log::WriteServerState("ServerState::LoadState");
        std::vector<model::Cell> cells;
        cells.reserve(States_.size());

        CellState *current = &Root_;
        uint32_t idx = 0;
        while (current) {
            if (!current->Next_) {
                log::WriteFullStateLog("ERROR [", idx, "] next is nullptr");
                std::exit(1);
            }
            log::WriteFullStateLog('[', idx, "] move from ", current->CellId_, " to ", current->Next_->CellId_);
            current = current->Next_;

            if (current == &Root_) {
                break;
            }
            if (current->Deleted_) {
                log::WriteFullStateLog('[', idx, "] skip ", current->CellId_);
                continue;
            }
            log::WriteFullStateLog('[', idx++, "] add ", current->CellId_);
            cells.push_back(*current);
        }

        log::WriteServerState("ServerState::LoadState{cells.size()=", cells.size(), '}');
        return api::State(std::move(cells));
    }

    void Connect(model::ClientId _id) override {
        log::WriteServerState("ServerState::Connect");
    }

    void GetNextHistory(model::ClientId _id, api::GenericResponse *response) override {
        log::WriteServerState("ServerState::GetNextHistory");
        model::IterationI lastSeenIteration = LastSeenClients_[_id];
        if (History_.size() < Iteration_ - lastSeenIteration) {
            log::WriteServerState("ServerState::GetNextHistory{Old}");
            return;
        }
        uint64_t beginIdx = lastSeenIteration - LastCutIteration_;
        auto begin = History_.begin() + beginIdx;
        for (auto it = begin; it != History_.end(); ++it) {
            auto put = [&] (auto &cmd) {
                if constexpr (std::is_same_v<model::InsertValue, std::decay_t<decltype(cmd)>>) {
                    CellState &state = GetState(cmd.Cell_.CellId_);
                    response->Insertions_.push_back(cmd);
                    if (state.Prev_->Deleted_) {
                        response->Insertions_.back().NearCellId_ = state.FindNear()->CellId_;
                    }
                    log::WriteHistoryLog(">>insert<< ", cmd.Cell_.CellId_, ' ', cmd.NearCellId_);
                }
                if constexpr (std::is_same_v<model::UpdateValue, std::decay_t<decltype(cmd)>>) {
                    log::WriteHistoryLog(">>update<< ", cmd.Cell_.CellId_, ' ', cmd.Cell_.Value_);
                    CellState &state = GetState(cmd.Cell_.CellId_);
                    response->Updates_.push_back(cmd);
                    response->Updates_.back().Cell_ = state;
                }
                if constexpr (std::is_same_v<model::DeleteValue, std::decay_t<decltype(cmd)>>) {
                    log::WriteHistoryLog(">>delete<< ", cmd.CellId_);
                    response->Deletions_.push_back(cmd);
                }
            };
            std::visit(put, *it);
        }
    }

    model::IterationI GetIteration() override {
        return Iteration_;
    }

    void ApplyHistory(model::IterationI _toIteration) {
        log::WriteServerState("ServerState::ApplyHistory ", _toIteration);
        auto update = [&] (auto cmd) {
            using type = std::decay_t<decltype(cmd)>;
            if constexpr (std::is_same_v<type, model::UpdateValue>) {
                log::WriteServerState("ServerState::ApplyHistory{UpdateValue} ", cmd.Cell_.CellId_, ' ', cmd.Cell_.Value_);
                CellState &state = GetState(cmd.Cell_.CellId_);
                state.Unref();
            }
            if constexpr (std::is_same_v<type, model::InsertValue>) {
                log::WriteServerState("ServerState::ApplyHistory{InsertValue} ", cmd.Cell_.CellId_);
                CellState &state = GetState(cmd.Cell_.CellId_);
                state.Unref();
            }
            if constexpr (std::is_same_v<type, model::DeleteValue>) {
                log::WriteServerState("ServerState::ApplyHistory{DeleteValue ", cmd.CellId_, "}");
                CellState &state = GetState(cmd.CellId_);
                if (&state == Root_.NearLive_) {
                    Root_.FindNear();
                }
                state.Unref();
                if (state.Next_->NearLive_ == &state) {
                    state.Next_->SetNear(state.NearLive_);
                }
            }
        };
        if (_toIteration <= LastCutIteration_) {
            return;
        }
        uint64_t end = _toIteration - LastCutIteration_;
        if (History_.size() < end) {
            log::WriteServerState("ServerState::ApplyHistory{Overflow}");
        }
        for (uint64_t idx = 0; idx < end; ++idx) {
            std::visit(update, History_.front());
            History_.pop_front();
        }
        LastCutIteration_ = _toIteration;

    }

    void MoveIterationForClient(model::ClientId _id, model::IterationI _iteration) override {
        log::WriteServerState("ServerState::MoveIterationForClient ", _id, ' ', _iteration);
        auto it = LastSeenClients_.find(_id);
        if (it == LastSeenClients_.end()) {
            log::WriteServerState("ServerState::MoveIterationForClient{New}");
            LastSeenClients_[_id] = _iteration;
            OrderedSeenClients_.emplace(_iteration, _id);
            return;
        }
        it->second = _iteration;
        log::WriteServerState("ServerState::MoveIterationForClient{Clean}");
        while (OrderedSeenClients_.size()) {
            auto [iteration, id] = OrderedSeenClients_.top();
            model::IterationI currentLastSeenIteration = LastSeenClients_[id];
            if (currentLastSeenIteration == iteration) {
                break;
            }
            OrderedSeenClients_.pop();
            OrderedSeenClients_.emplace(currentLastSeenIteration, id);

        }
        log::WriteServerState("ServerState::MoveIterationForClient{Cleaned}");
    }

    void CutHistory() override {
        log::WriteServerState("ServerState::CutHistory");
        if (OrderedSeenClients_.size()) {
            model::IterationI iteration = OrderedSeenClients_.top().first;
            if (iteration) {
                log::WriteServerState("ServerState::CutHistory{ApplyHistory}");
                ApplyHistory(OrderedSeenClients_.top().first);
            }
        }
        if (QueueToRemove_.size()) {
            CleanQueue();
        }
    }
};

}
