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
    std::priority_queue<
            std::pair<uint64_t, uint32_t>,
            std::vector<std::pair<uint64_t, uint32_t>>,
            std::greater<std::pair<uint64_t, uint32_t>>>
        OrderedSeenClients_;

    std::unordered_map<uint64_t, decltype(std::declval<model::Cell>().Value_)> PostponedUpdate_;
    std::unordered_map<uint64_t, uint64_t> PostponedUpdateCnt_;
    std::unordered_map<uint64_t, std::deque<api::GenericResponse::InsertValue*>> PostponedInsertion_;
    std::unordered_map<uint64_t, uint64_t> PostponedDeletion_;

    uint64_t NextCellId_ = 1;
    
    ServerState(const std::vector<model::Cell> &_cells) {
        log::WriteServerState("ServerState{_cells.size()=", _cells.size(), '}');
        Cells_.insert(Cells_.begin(), _cells.cbegin(), _cells.cend());
        uint64_t idx = 0;
        for (auto it = Cells_.begin(); it != Cells_.end(); ++it, ++idx) {
            Ids_[it->CellId_] = it;
            NextCellId_ = std::max(NextCellId_, it->CellId_ + 1);
            log::WriteInit("ServerInit idx=", idx, " id=", it->CellId_);
        }
    }

    virtual ~ServerState(){}

    api::UpdateValueResponse UpdateValue(const api::UpdateValueRequest &_request) override {
        log::WriteServerState("ServerState::UpdateValue ", _request.CellId_, ' ',_request.Value_);
        if (!PostponedDeletion_.count(_request.CellId_)) {
            Iteration_++;
            model::Cell cell(_request.CellId_, _request.Value_);
            History_.emplace_back(api::GenericResponse::UpdateValue{.Cell_ = cell});
            PostponedUpdate_[_request.CellId_] = _request.Value_;
            PostponedUpdateCnt_[_request.CellId_]++;
            log::WriteServerState("ServerState::InsertValue{postponed waited ", PostponedUpdateCnt_[_request.CellId_],"}");
        }
        return api::UpdateValueResponse();
    }

    api::InsertValueResponse InsertValue(const api::InsertValueRequest &_request) override {
        log::WriteServerState("ServerState::InsertValue");
        Iteration_++;
        uint64_t cellId = NextCellId_++;
        model::Cell cell(cellId, _request.Value_);
        uint64_t nearCellId = _request.NearCellId_;
        auto it = PostponedDeletion_.find(nearCellId);
        if (it != PostponedDeletion_.end()) {
            nearCellId = it->second;
            log::WriteServerState("ServerState::InsertValue{with deleted nearCell}");
        }
        History_.emplace_back(api::GenericResponse::InsertValue{.NearCellId_ = nearCellId, .Cell_ = cell});
        PostponedInsertion_[nearCellId].push_front(&std::get<api::GenericResponse::InsertValue>(*--History_.end()));
        return api::InsertValueResponse(cellId);
    }

    api::DeleteValueResponse DeleteValue(const api::DeleteValueRequest &_request) override {
        log::WriteServerState("ServerState::DeleteValue");
        auto delIt = PostponedDeletion_.find(_request.CellId_);
        if (delIt == PostponedDeletion_.end()) {
            Iteration_++;
            History_.emplace_back(api::GenericResponse::DeleteValue{.CellId_ = _request.CellId_});
            auto listIterator = Cells_.begin();
            uint64_t toCellId = 0;
            if (auto it = Ids_.find(_request.CellId_); it != Ids_.end() && it->second != Cells_.begin()) {
                auto listIterator = it->second;
                toCellId = (--listIterator)->CellId_;
                listIterator++;
            }
            PostponedDeletion_[_request.CellId_] = toCellId;
            listIterator++;
            while (listIterator != Cells_.end()) {
                auto deletionIt = PostponedDeletion_.find(listIterator->CellId_);
                if (deletionIt == PostponedDeletion_.end()) {
                    break;
                }
                log::WriteServerState("ServerState::DeleteValue{shift}");
                deletionIt->second = toCellId;
                listIterator++;
            }
        }
        return api::DeleteValueResponse();
    }

    void CheckUpdate(uint64_t _cellId, auto &_cells, uint64_t *_idx) {
        auto updateIt = PostponedUpdate_.find(_cellId);
        if (updateIt != PostponedUpdate_.end()) {
            log::WriteFullStateLog('[', *_idx, "] update value ", _cellId, ' ', updateIt->second);
            _cells.back().Value_ = updateIt->second;
        }
    }

    void AddPostponed(uint64_t _cellId, auto &_cells, uint64_t *_idx) {
        auto insertIt = PostponedInsertion_.find(_cellId);
        if (insertIt != PostponedInsertion_.end()) {
            for (auto &nextCellIt : insertIt->second) {
                if (!PostponedDeletion_.count(nextCellIt->Cell_.CellId_)) {
                    _cells.push_back(nextCellIt->Cell_);
                    CheckUpdate(nextCellIt->Cell_.CellId_, _cells, _idx);
                    log::WriteFullStateLog('[', *_idx, "] add postponed ", nextCellIt->Cell_.CellId_);
                    (*_idx)++;
                    AddPostponed(nextCellIt->Cell_.CellId_, _cells, _idx);
                } else {
                    log::WriteFullStateLog('[', *_idx, "] skip postponed ", nextCellIt->Cell_.CellId_);
                }
            }
        }
    }

    api::State LoadState() override {
        log::WriteServerState("ServerState::LoadState");
        std::vector<model::Cell> cells;
        cells.reserve(Cells_.size() + PostponedInsertion_.size() - PostponedDeletion_.size());
        uint64_t idx = 0;

        AddPostponed(0, cells, &idx);
        for (auto &cell : Cells_) {
            if (!PostponedDeletion_.count(cell.CellId_)) {
                cells.push_back(cell);
                CheckUpdate(cell.CellId_, cells, &idx);
                log::WriteFullStateLog('[', idx, "] add ", cell.CellId_);
                idx++;
            } else {
                log::WriteFullStateLog('[', idx, "] skip ", cell.CellId_);
            }
            AddPostponed(cell.CellId_, cells, &idx);
        }
        log::WriteServerState("ServerState::LoadState{cells.size()=", cells.size(), '}');
        return api::State(std::move(cells));
    }

    void Connect(uint32_t _id) override {
        log::WriteServerState("ServerState::Connect");
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
        for (auto it = begin; it != History_.end(); ++it) {
            auto change = [&] (auto &cmd) {
                if constexpr (std::is_same_v<api::GenericResponse::InsertValue, std::decay_t<decltype(cmd)>>) {
                    log::WriteFullStateLog(">>insert<< ", cmd.NearCellId_, ' ', cmd.Cell_.CellId_);
                    auto delIt = PostponedDeletion_.find(cmd.NearCellId_);
                    if (delIt != PostponedDeletion_.end()) {
                        cmd.NearCellId_ = delIt->second;
                    }
                }
                if constexpr (std::is_same_v<api::GenericResponse::UpdateValue, std::decay_t<decltype(cmd)>>) {
                    log::WriteFullStateLog(">>update<< ", cmd.Cell_.CellId_, ' ', cmd.Cell_.Value_);
                }
                if constexpr (std::is_same_v<api::GenericResponse::DeleteValue, std::decay_t<decltype(cmd)>>) {
                    log::WriteFullStateLog(">>delete<< ", cmd.CellId_);
                }
            };
            result.push_back(*it);
            std::visit(change, result.back());
        }
        return std::move(result);
    }

    uint64_t GetIteration() override {
        return Iteration_;
    }

    void ApplyHistory(uint64_t _toIteration) {
        log::WriteServerState("ServerState::ApplyHistory ", _toIteration);
        auto update = [&] (auto cmd) {
            using type = std::decay_t<decltype(cmd)>;
            if constexpr (std::is_same_v<type, api::GenericResponse::UpdateValue>) {
                log::WriteServerState("ServerState::ApplyHistory{UpdateValue} ", cmd.Cell_.CellId_, ' ', cmd.Cell_.Value_);
                auto idIt = Ids_.find(cmd.Cell_.CellId_);
                if (idIt == Ids_.end()) {
                    log::WriteServerState("ServerState::ApplyHistory{can't find cell}");
                    std::exit(1);
                }
                auto itCell = idIt->second;
                itCell->Value_ = cmd.Cell_.Value_;
                if (!--PostponedUpdateCnt_[cmd.Cell_.CellId_]) {
                    log::WriteServerState("ServerState::ApplyHistory{erase from postponed} ", cmd.Cell_.CellId_);
                    PostponedUpdate_.erase(cmd.Cell_.CellId_);
                    PostponedUpdateCnt_.erase(cmd.Cell_.CellId_);
                } else {
                    log::WriteServerState("ServerState::ApplyHistory{waited post} ", PostponedUpdateCnt_[cmd.Cell_.CellId_]);
                }
            }
            if constexpr (std::is_same_v<type, api::GenericResponse::InsertValue>) {
                log::WriteServerState("ServerState::ApplyHistory{InsertValue} ", cmd.Cell_.CellId_);
                auto cellIt = Cells_.begin();
                if (cmd.NearCellId_) {
                    auto idIt = Ids_.find(cmd.NearCellId_);
                    if (idIt == Ids_.end()) {
                        log::WriteServerState("ServerState::ApplyHistory{can't find cell ", cmd.NearCellId_,"}");
                    }
                    cellIt = idIt->second;
                    cellIt++;
                }
                Ids_[cmd.Cell_.CellId_] = Cells_.insert(cellIt, cmd.Cell_);
                auto insertionId = PostponedInsertion_.find(cmd.NearCellId_);
                if (insertionId == PostponedInsertion_.end()) {
                    log::WriteServerState("ServerState::ApplyHistory{can't find waitedInsertion cell} ", cmd.NearCellId_);
                }
                log::WriteServerState("ServerState::ApplyHistory{pop from postponed} ", insertionId->second.back()->Cell_.CellId_);
                insertionId->second.pop_back();
                if (insertionId->second.empty()) {
                    PostponedInsertion_.erase(insertionId);
                    log::WriteServerState("ServerState::ApplyHistory{remove PostponedInsertion_} ", cmd.NearCellId_);
                }
            }
            if constexpr (std::is_same_v<type, api::GenericResponse::DeleteValue>) {
                log::WriteServerState("ServerState::ApplyHistory{DeleteValue ", cmd.CellId_, "}");
                auto idIt = Ids_.find(cmd.CellId_);
                if (idIt == Ids_.end()) {
                    log::WriteServerState("ServerState::ApplyHistory{can't find cell}");
                    std::exit(1);
                }
                auto cellIt = idIt->second;
                if (cellIt != Cells_.begin()) {
                    cellIt--;
                    log::WriteServerState("ServerState::ApplyHistory{left neigh} ", cellIt->CellId_);
                    cellIt++;
                } else {
                    log::WriteServerState("ServerState::ApplyHistory{left neigh} ", 0);
                }
                Cells_.erase(cellIt);
                log::WriteServerState("ServerState::ApplyHistory{erased from Cells_} ");
                PostponedDeletion_.erase(cmd.CellId_);
                log::WriteServerState("ServerState::ApplyHistory{erased from PostponedDeletion_} ");
                Ids_.erase(cmd.CellId_);
                log::WriteServerState("ServerState::ApplyHistory{erased from Ids_} ");
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
        log::WriteServerState("ServerState::CutHistory");
        if (OrderedSeenClients_.size()) {
            uint64_t iteration = OrderedSeenClients_.top().first;
            if (iteration) {
                log::WriteServerState("ServerState::CutHistory{ApplyHistory}");
                ApplyHistory(OrderedSeenClients_.top().first);
            }
        }
    }
};

}
