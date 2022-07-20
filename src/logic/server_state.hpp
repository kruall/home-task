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

    virtual void Connect(uint32_t _id) = 0;

    virtual std::vector<api::GenericResponse::Modification> GetNextHistory(uint32_t _id) = 0;

    virtual uint64_t GetIteration() = 0;

    virtual void MoveIterationForClient(uint32_t _id, uint64_t _iteration) = 0;

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

    void Connect(uint32_t _id) override {
        log::WriteServerState("ServerStateNop::LoadState");
    }

    std::vector<api::GenericResponse::Modification> GetNextHistory(uint32_t) override {
        log::WriteServerState("ServerStateNop::GetNextHistory");
        return {};
    }
    
    uint64_t GetIteration() override {
        return 0;
    }

    void MoveIterationForClient(uint32_t, uint64_t) override {
        log::WriteServerState("ServerStateNop::MoveIterationForClient");
        return;
    }

    void CutHistory() override {

    }
};

struct ServerState : IServerState {
    std::list<model::Cell> Cells_;
    std::unordered_map<uint64_t, std::list<model::Cell>::iterator> Ids_;

    std::deque<api::GenericResponse::Modification> History_;
    uint64_t LastCutIteration_ = 0;
    uint64_t Iteration_ = 0;

    std::unordered_map<uint32_t, uint64_t> LastSeenClients_;
    std::priority_queue<std::pair<uint64_t, uint32_t>> OrderedSeenClients_;

    std::unordered_map<uint64_t, decltype(std::declval<model::Cell>().Value_)> WaitedUpdate_;
    std::unordered_map<uint64_t, std::deque<model::Cell>> WaitedInsertion_;
    std::unordered_map<uint64_t, uint64_t> WaitedDeletion_;

    uint64_t NextCellId_ = 1;
    
    ServerState(const std::vector<model::Cell> &_cells) {
        log::WriteServerState("ServerState{_cells.size()=", _cells.size(), '}');
        Cells_.insert(Cells_.begin(), _cells.cbegin(), _cells.cend());
        for (auto it = Cells_.begin(); it != Cells_.end(); ++it) {
            Ids_[it->CellId_] = it;
            NextCellId_ = std::max(NextCellId_, it->CellId_ + 1);
        }
    }

    virtual ~ServerState(){}

    api::UpdateValueResponse UpdateValue(const api::UpdateValueRequest &_request) override {
        log::WriteServerState("ServerState::UpdateValue");
        Iteration_++;
        model::Cell cell(_request.CellId_, _request.Value_);
        History_.emplace_back(api::GenericResponse::UpdateValue{.Cell_ = cell});
        WaitedUpdate_[_request.CellId_] = _request.Value_;
        return api::UpdateValueResponse();
    }

    api::InsertValueResponse InsertValue(const api::InsertValueRequest &_request) override {
        log::WriteServerState("ServerState::InsertValue");
        Iteration_++;
        uint64_t cellId = NextCellId_++;
        model::Cell cell(cellId, _request.Value_);
        uint64_t nearCellId = _request.NearCellId_;
        auto it = WaitedDeletion_.find(nearCellId);
        if (it != WaitedDeletion_.end()) {
            nearCellId = it->second;
        }
        History_.emplace_back(api::GenericResponse::InsertValue{.NearCellId_ = nearCellId, .Cell_ = cell});
        WaitedInsertion_[nearCellId].push_back(cell);
        return api::InsertValueResponse(cellId);
    }

    api::DeleteValueResponse DeleteValue(const api::DeleteValueRequest &_request) override {
        log::WriteServerState("ServerState::DeleteValue");
        Iteration_++;
        History_.emplace_back(api::GenericResponse::DeleteValue{.CellId_ = _request.CellId_});
        auto listIterator = Cells_.begin();
        uint64_t toCellId = 0;
        if (auto it = Ids_.find(_request.CellId_); it != Ids_.end() && it->second != Cells_.begin()) {
            auto listIterator = it->second;
            toCellId = (--listIterator)->CellId_;
            listIterator++;
        }
        WaitedDeletion_[_request.CellId_] = toCellId;
        listIterator++;
        while (listIterator != Cells_.end()) {
            auto deletionIt = WaitedDeletion_.find(listIterator->CellId_);
            if (deletionIt == WaitedDeletion_.end()) {
                break;
            }
            deletionIt->second = toCellId;
            listIterator++;
        }
        return api::DeleteValueResponse();
    }

    api::State LoadState() override {
        log::WriteServerState("ServerState::LoadState");
        std::vector<model::Cell> cells;
        cells.reserve(Cells_.size() + WaitedInsertion_.size() - WaitedDeletion_.size());
        for (auto &cell : Cells_) {
            if (!WaitedDeletion_.count(cell.CellId_)) {
                cells.push_back(cell);
                auto updateIt = WaitedUpdate_.find(cell.CellId_);
                if (updateIt != WaitedUpdate_.end()) {
                    cells.back().Value_ = updateIt->second;
                }
            }
            auto insertIt = WaitedInsertion_.find(cell.CellId_);
            if (insertIt != WaitedInsertion_.end()) {
                for (auto &nextCell : insertIt->second) {
                    cells.push_back(nextCell);
                }
            }
        }
        log::WriteServerState("ServerState::LoadState{cells.size()=", cells.size(), '}');
        return api::State(std::move(cells));
    }

    void Connect(uint32_t _id) override {
        log::WriteServerState("ServerState::LoadState");
    }

    std::vector<api::GenericResponse::Modification> GetNextHistory(uint32_t _id) override {
        log::WriteServerState("ServerState::GetNextHistory");
        std::vector<api::GenericResponse::Modification> result;
        uint64_t lastSeenIteration = LastSeenClients_[_id];
        if (History_.size() < Iteration_ - lastSeenIteration) {
            log::WriteServerState("ServerState::GetNextHistory{Old}");
            return {};
        }
        uint64_t beginIdx = lastSeenIteration - LastCutIteration_;
        result.reserve(History_.size() - beginIdx);
        auto begin = History_.begin() + beginIdx;
        result.insert(result.end(), begin, History_.end());
        return std::move(result);
    }

    uint64_t GetIteration() override {
        return Iteration_;
    }

    void Update(uint64_t _toIteration) {
        log::WriteServerState("ServerState::Update ", _toIteration);
        auto update = [&] (auto cmd) {
            using type = std::decay_t<decltype(cmd)>;
            if constexpr (std::is_same_v<type, api::GenericResponse::UpdateValue>) {
                log::WriteServerState("ServerState::Update{UpdateValue}");
                auto idIt = Ids_.find(cmd.Cell_.CellId_);
                if (idIt == Ids_.end()) {
                    log::WriteServerState("ServerState::Update{can't find cell}");
                }
                auto itCell = idIt->second;
                itCell->Value_ = cmd.Cell_.Value_;
                WaitedUpdate_.erase(cmd.Cell_.CellId_);
            }
            if constexpr (std::is_same_v<type, api::GenericResponse::InsertValue>) {
                log::WriteServerState("ServerState::Update{InsertValue}");
                auto cellIt = Cells_.begin();
                if (cmd.NearCellId_) {
                    auto idIt = Ids_.find(cmd.NearCellId_);
                    if (idIt == Ids_.end()) {
                        log::WriteServerState("ServerState::Update{can't find cell ", cmd.NearCellId_,"}");
                    }
                    cellIt = idIt->second;
                    cellIt++;
                }
                Ids_[cmd.Cell_.CellId_] = Cells_.insert(cellIt, cmd.Cell_);
                auto wit = WaitedInsertion_.find(cmd.NearCellId_);
                if (wit == WaitedInsertion_.end()) {
                    log::WriteServerState("ServerState::Update{can't find waitedInsertion cell}");
                }
                wit->second.pop_front();
                if (wit->second.empty()) {
                    WaitedInsertion_.erase(wit);
                }
            }
            if constexpr (std::is_same_v<type, api::GenericResponse::DeleteValue>) {
                log::WriteServerState("ServerState::Update{DeleteValue ", cmd.CellId_, "}");
                auto idIt = Ids_.find(cmd.CellId_);
                if (idIt == Ids_.end()) {
                    log::WriteServerState("ServerState::Update{can't find cell}");
                }
                auto cellIt = idIt->second;
                Cells_.erase(cellIt);
                WaitedDeletion_.erase(cmd.CellId_);
                Ids_.erase(cmd.CellId_);
            }
        };
        uint64_t end = _toIteration - LastCutIteration_;
        if (History_.size() < end) {
            log::WriteServerState("ServerState::Update{Overflow}");
        }
        for (uint64_t idx = 0; idx < end; ++idx) {
            std::visit(update, History_.front());
            History_.pop_front();
        }
        LastCutIteration_ = _toIteration;

    }

    void MoveIterationForClient(uint32_t _id, uint64_t _iteration) override {
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
            uint64_t currentLastSeenIteration = LastSeenClients_[id];
            if (currentLastSeenIteration == iteration) {
                break;
            }
            OrderedSeenClients_.pop();
            OrderedSeenClients_.emplace(currentLastSeenIteration, id);

        }
        log::WriteServerState("ServerState::MoveIterationForClient{Cleaned}");
    }

    void CutHistory() override {
        if (OrderedSeenClients_.size()) {
            Update(OrderedSeenClients_.top().first);
        }
    }
};

}
