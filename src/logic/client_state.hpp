#pragma once

#include <core/api.hpp>

#include <algorithm>
#include <random>
#include <deque>
#include <cassert>
#include <unordered_set>
#include <cstdlib>


namespace home_task::logic {

struct IClientState {
    virtual ~IClientState() {}

    virtual api::UpdateValueRequest GenerateUpdateValueRequest(std::mt19937 &_gen) = 0;

    virtual api::InsertValueRequest GenerateInsertValueRequest(std::mt19937 &_gen) = 0;

    virtual api::DeleteValueRequest GenerateDeleteValueRequest(std::mt19937 &_gen) = 0;

    virtual api::LoadStateRequest GenerateLoadStateRequest() = 0;

    virtual api::SyncRequest GenerateSyncRequest() = 0;

    virtual void HandleUpdateValueResponse(const api::UpdateValueResponse &_response) = 0;

    virtual void HandleInsertValueResponse(const api::InsertValueResponse &_response) = 0;

    virtual void HandleDeleteValueResponse(const api::DeleteValueResponse &_response) = 0;

    virtual void HandleSyncResponse(const api::SyncResponse &_response) = 0;

    virtual void HandleState(const api::State &_response) = 0;

};

struct ClientStateNop : IClientState {
    virtual ~ClientStateNop(){}

    api::UpdateValueRequest GenerateUpdateValueRequest(std::mt19937 &) override {
        log::WriteClientState("ClientStateNop::GenerateUpdateValueRequest");
        return api::UpdateValueRequest(0, 0, 0);
    }

    api::InsertValueRequest GenerateInsertValueRequest(std::mt19937 &) override {
        log::WriteClientState("ClientStateNop::GenerateInsertValueRequest");
        return api::InsertValueRequest(0, 0, 0);
    }

    api::DeleteValueRequest GenerateDeleteValueRequest(std::mt19937 &) override {
        log::WriteClientState("ClientStateNop::GenerateDeleteValueRequest");
        return api::DeleteValueRequest(0, 0);
    }

    api::LoadStateRequest GenerateLoadStateRequest() override {
        log::WriteClientState("ClientStateNop::GenerateLoadState");
        return api::LoadStateRequest(0);
    }

    api::SyncRequest GenerateSyncRequest() override {
        log::WriteClientState("ClientStateNop::GenerateSyncRequest");
        return api::SyncRequest(0);
    }

    void HandleUpdateValueResponse(const api::UpdateValueResponse &) override {
        log::WriteClientState("ClientStateNop::HandleUpdateValueResponse");
    }

    void HandleInsertValueResponse(const api::InsertValueResponse &) override {
        log::WriteClientState("ClientStateNop::HandleInsertValueResponse");
    }

    void HandleDeleteValueResponse(const api::DeleteValueResponse &) override {
        log::WriteClientState("ClientStateNop::HandleDeleteValueResponse");
    }

    void HandleSyncResponse(const api::SyncResponse &_response) override {
        log::WriteClientState("ClientStateNop::HandleSyncResponse");
    }

    void HandleState(const api::State &) override {
        log::WriteClientState("ClientStateNop::HandleState");
    }
};



struct ClientState : IClientState {
    std::deque<model::Cell> Cells_;
    model::IterationI Iteration_ = 0;

    virtual ~ClientState(){}

    api::UpdateValueRequest GenerateUpdateValueRequest(std::mt19937 &_gen) override {
        std::uniform_int_distribution<uint32_t> idxDistrib(0, Cells_.size()-1);
        auto &cell = Cells_[idxDistrib(_gen)];
        std::uniform_int_distribution<model::Value> valueDistrib(0);
        model::Value value = valueDistrib(_gen);
        lcog::WriteClientState("ClientState::GenerateUpdateValueRequest{id=", cell.CellId_, ", value=", value, '}');
        return api::UpdateValueRequest(cell.CellId_, value, Iteration_);
    }

    api::InsertValueRequest GenerateInsertValueRequest(std::mt19937 &_gen) override {
        std::uniform_int_distribution<uint32_t> idxDistrib(0, Cells_.size());
        model::CellId cellId = 0;
        if (uint32_t idx = idxDistrib(_gen)) {
            cellId = Cells_[idx - 1].CellId_;
        }
        std::uniform_int_distribution<model::Value> valueDistrib(0);
        model::Value value = valueDistrib(_gen);
        log::WriteClientState("ClientState::GenerateInsertValueRequest{after=", cellId, ", value=", value, '}');
        return api::InsertValueRequest(cellId, value, Iteration_);
    }

    api::DeleteValueRequest GenerateDeleteValueRequest(std::mt19937 &_gen) override {
        std::uniform_int_distribution<uint32_t> idxDistrib(0, Cells_.size()-1);
        uint32_t idx = idxDistrib(_gen);
        log::WriteClientState("ClientState::GenerateDeleteValueRequest{id=", Cells_[idx].CellId_, '}');
        return api::DeleteValueRequest(Cells_[idx].CellId_, Iteration_);
    }

    api::SyncRequest GenerateSyncRequest() override {
        log::WriteClientState("ClientState::GenerateSyncRequest");
        return api::SyncRequest(Iteration_);
    }

    api::LoadStateRequest GenerateLoadStateRequest() override {
        log::WriteClientState("ClientState::GenerateLoadState");
        return api::LoadStateRequest(Iteration_);
    }

    void ApplyOperations(const api::GenericResponse &_response) {
        std::unordered_map<model::CellId, const model::InsertValue*> postponedInserts;
        for (auto &update : _response.Updates_) {
            auto it = std::find_if(Cells_.begin(), Cells_.end(), [id=update.Cell_.CellId_] (auto &el) { return el.CellId_ == id;});
            it->Value_ = update.Cell_.Value_;
        }
        for (auto &insert : _response.Insertions_) {
            if (insert.NearCellId_) {
                auto it = std::find_if(Cells_.begin(), Cells_.end(), [id=insert.NearCellId_] (auto &el) { return el.CellId_ == id;});
                if (it == Cells_.end()) {
                    postponedInserts[insert.NearCellId_] = &insert;
                } else {
                    it++;
                    Cells_.insert(it, insert.Cell_);
                }
            } else {
                Cells_.insert(Cells_.begin(), insert.Cell_);
            }
            auto postponedIt = postponedInserts.find(insert.Cell_.CellId_);
            while (postponedIt != postponedInserts.end()) {
                auto it = std::find_if(Cells_.begin(), Cells_.end(), [id=postponedIt->first] (auto &el) { return el.CellId_ == id;});
                it++;
                Cells_.insert(it, postponedIt->second->Cell_);
                auto next = postponedInserts.find(postponedIt->second->Cell_.CellId_);
                postponedInserts.erase(std::exchange(postponedIt, next));
            }
        }
        for (auto &del : _response.Deletions_) {
            auto it = std::find_if(Cells_.begin(), Cells_.end(), [id=del.CellId_] (auto &el) { return el.CellId_ == id;});
            Cells_.erase(it);
        }
    }

    void HandleUpdateValueResponse(const api::UpdateValueResponse &_response) override {
        log::WriteClientState("ClientState::HandleUpdateValueResponse");
        ApplyOperations(_response);
        Iteration_ = _response.Iteration_;
    }

    void HandleInsertValueResponse(const api::InsertValueResponse &_response) override {
        log::WriteClientState("ClientState::HandleInsertValueResponse");
        ApplyOperations(_response);
        Iteration_ = _response.Iteration_;
    }

    void HandleDeleteValueResponse(const api::DeleteValueResponse &_response) override {
        log::WriteClientState("ClientState::HandleDeleteValueResponse");
        ApplyOperations(_response);
        Iteration_ = _response.Iteration_;
    }

    void HandleSyncResponse(const api::SyncResponse &_response) override {
        log::WriteClientState("ClientState::HandleSyncResponse");
        ApplyOperations(_response);
        Iteration_ = _response.Iteration_;
    }

    void HandleState(const api::State &_response) override {
        log::WriteClientState("ClientState::HandleState");
        if (Cells_.empty()) {
            log::WriteClientState("ClientState::HandleState{Init}");
            Cells_.insert(Cells_.begin(), _response.Cells_.cbegin(), _response.Cells_.cend());
            log::WriteClientState("ClientState::HandleState{Cells_.size()=", Cells_.size(),'}');
            return;
        }
        if (magic_numbers::WithStateChecking) {
            ApplyOperations(_response);
            if (Cells_.size() != _response.Cells_.size()) {
                log::ForceWrite("Sizes aren't equal ", Cells_.size(), ' ', _response.Cells_.size());
                std::exit(1);
            }
            uint32_t idx = 0;
            for (auto &cell : Cells_) {
                if (cell != _response.Cells_[idx]) {
                    if (cell.CellId_ !=  _response.Cells_[idx].CellId_) {
                        log::ForceWrite("Cells aren't equal at ", idx, ' ', cell.CellId_, ' ', _response.Cells_[idx].CellId_);
                    } else {
                        log::ForceWrite("Cells equal by id at ", idx, ' ', cell.CellId_, ' ', _response.Cells_[idx].CellId_);
                        log::ForceWrite("Cells aren't equal by value at ", idx, ' ', cell.Value_, ' ', _response.Cells_[idx].Value_);
                    }
                    for (uint32_t idx2 = 0; idx2 < Cells_.size(); ++idx2) {
                        log::WriteFullStateLog('<', idx2, "> ", Cells_[idx2].CellId_);
                    }
                    log::ForceWrite("END LIST");
                    std::exit(1);
                }
                idx++;
            }
            Iteration_ = _response.Iteration_;
        }
    }
};

template <typename _Value>
struct DecardTree {
    using Pointer = DecardTree*;
    using ConstPointer = const DecardTree*;
    using Priority = uint64_t;

    Pointer Left_ = nullptr;
    Pointer Right_ = nullptr;
    Pointer Parent_ = nullptr;

    uint32_t Size_ = 1;
    Priority Priority_ = 0;
    
    _Value Value_;

    ~DecardTree() {
        if (Left_) {
            delete Left_;
        }
        if (Right_) {
            delete Right_;
        }
    }

    uint32_t GetLeftSize() const {
        if (Left_) {
            return Left_->Size_;
        }
        return 0;
    }

    uint32_t GetRightSize() const {
        if (Right_) {
            return Right_->Size_;
        }
        return 0;
    }

    void TieLeft() {
        if (Left_) {
            Left_->Parent_ = this;
        }
    }

    Pointer UntieLeft() {
        if (Left_) {
            Left_->Parent_ = nullptr;
        }
        return std::exchange(Left_, nullptr);
    }


    void TieRight() {
        if (Right_) {
            Right_->Parent_ = this;
        }
    }

    Pointer UntieRight() {
        if (Right_) {
            Right_->Parent_ = nullptr;
        }
        return std::exchange(Right_, nullptr);
    }

    void RecalculateSize() {
        Size_ = 1 + GetLeftSize() + GetRightSize();
    }

    std::pair<Pointer, Pointer> SplitByIndex(uint32_t _idx) {
        log::WriteDecardTree("SplitByIndex ", _idx);
        if (_idx > Size_) {
            log::WriteDecardTree("SplitByIndex{TooBig} ", _idx, '/', Size_);
        }
        if (_idx == Size_) {
            log::WriteDecardTree("SplitByIndex{Big} ", _idx);
            Parent_ = nullptr;
            RecalculateSize();
            return {this, nullptr};
        }
        uint32_t leftSize = GetLeftSize();
        if (leftSize < _idx) {
            log::WriteDecardTree("SplitByIndex{Right} ", _idx);
            if (Right_) {
                auto [left, right] = UntieRight()->SplitByIndex(_idx - leftSize - 1);
                Right_ = left;
                TieRight();
                Parent_ = nullptr;
                if (right) right->Parent_ = nullptr;
                RecalculateSize();
                if (right) right->RecalculateSize();
                return {this, right};
            } else {
                Parent_ = nullptr;
                RecalculateSize();
                return {this, nullptr};
            }
        } else {
            log::WriteDecardTree("SplitByIndex{Left} ", _idx);
            if (Left_) {
                auto [left, right] = Left_->SplitByIndex(_idx);
                Left_ = right;
                TieLeft();
                Parent_ = nullptr;
                if (left) left->Parent_ = nullptr;
                RecalculateSize();
                if (left) left->RecalculateSize();
                return {left, this};
            } else {
                Parent_ = nullptr;
                RecalculateSize();
                return {nullptr, this};
            }
        }
    }

    Pointer Insert(uint32_t _idx, Pointer _node) {
        log::WriteDecardTree("Insert ", _idx);
        if (!_node) {
            log::WriteDecardTree("Insert{Without node}");
        }
        if (_idx > Size_) {
            log::WriteDecardTree("Insert{TooBig}");
        }
        if (_node->Priority_ > Priority_) {
            log::WriteDecardTree("Insert{Up} ", _idx, ' ', GetLeftSize());
            auto [left, right] = SplitByIndex(_idx);
            _node->Left_ = left;
            _node->TieLeft();
            _node->Right_ = right;
            _node->TieRight();
            _node->Parent_ = nullptr;
            _node->RecalculateSize();
            return _node;
        } else if (_idx <= GetLeftSize()) {
            log::WriteDecardTree("Insert{Left} ", _idx, ' ', GetLeftSize());
            if (Left_) {
                Left_ = Left_->Insert(_idx, _node);
            } else {
                Left_ = _node;
            }
            TieLeft();
        } else {
            log::WriteDecardTree("Insert{Right} ", _idx, ' ', GetLeftSize());
            if (Right_) {
                Right_ = Right_->Insert(_idx - GetLeftSize() - 1, _node);
            } else {
                Right_ = _node;
            }
            TieRight();
        }
        RecalculateSize();
        Parent_ = nullptr;
        return this;
    }

    Pointer Merge(Pointer _tree) {
        log::WriteDecardTree("Merge");
        if (!this) {
            log::WriteDecardTree("Merge{not this}");
            std::exit(1);
            return this;
        }
        if (!_tree) {
            log::WriteDecardTree("Merge{not tree}");
            Parent_ = nullptr;
            log::WriteDecardTree("Merge{untie parrent}");
            RecalculateSize();
            log::WriteDecardTree("Merge{recalculate size}");
            return this;
        }
        if (Priority_ > _tree->Priority_) {
            log::WriteDecardTree("Merge{upper}");
            if (Right_) {
                Right_ = UntieRight()->Merge(_tree);
            } else {
                Right_ = _tree;
            }
            TieRight();
            Parent_ = nullptr;
            RecalculateSize();
            return this;
        } else {
            log::WriteDecardTree("Merge{lower}");
            if (_tree->Left_) {
                _tree->Left_ = Merge(_tree->UntieLeft());
            } else {
                _tree->Left_ = this;
            }
            _tree->TieLeft();
            _tree->Parent_ = nullptr;
            _tree->RecalculateSize();
            return _tree;
        }
    }

    uint32_t GetIdx() const {
        ConstPointer current = this;
        uint32_t acc = GetLeftSize();
        while (current->Parent_) {
            if (current->Parent_->Right_ == current) {
                acc += current->Parent_->GetLeftSize() + 1;
            }
            current = current->Parent_;
        }
        return acc;
    }

    Pointer Get(uint32_t _idx) {
        uint32_t leftSize = GetLeftSize();
        if (leftSize == _idx) {
            return this;
        } else {
            if (leftSize > _idx) {
                return Left_->Get(_idx);
            } else {
                return Right_->Get(_idx - leftSize - 1);
            }
        }
    }

    std::pair<Pointer, Pointer> Erase(uint32_t _idx) {
        uint32_t leftSize = GetLeftSize();
        log::WriteDecardTree("Erase ", _idx , '/', Size_, ' ', leftSize);
        if (leftSize == _idx) {
            auto res = Left_ ? UntieLeft()->Merge(UntieRight()) : UntieRight();
            log::WriteDecardTree("Erase{merged}");
            Parent_ = nullptr;
            Size_ = 1;
            log::WriteDecardTree("Erase{merged return}");
            return {res, this};
        } else {
            if (leftSize > _idx) {
                auto [left, erased] = UntieLeft()->Erase(_idx);
                Left_ = left;
                TieLeft();
                Size_--;
                return {this, erased};
            } else {
                auto [right, erased] = UntieRight()->Erase(_idx - leftSize - 1);
                Right_ = right;
                TieRight();
                Size_--;
                return {this, erased};
            }
        }
    }
};

struct FastClientState : IClientState {
    DecardTree<model::Cell> Cells_;
    std::unordered_map<model::CellId, DecardTree<model::Cell>::Pointer> Nodes_;

    std::random_device RandomDevice_;
    std::mt19937 Generator_;
    std::uniform_int_distribution<DecardTree<model::Cell>::Priority> GeneratePriority_;

    model::IterationId Iteration_ = 0;

    FastClientState()
        : Cells_(DecardTree<model::Cell>{.Priority_ = std::numeric_limits<DecardTree<model::Cell>::Priority>::max(), .Value_={0, 0}})
        , Generator_(RandomDevice_())
        , GeneratePriority_(0, std::numeric_limits<DecardTree<model::Cell>::Priority>::max() - 1)
    {}

    virtual ~FastClientState(){}

    api::UpdateValueRequest GenerateUpdateValueRequest(std::mt19937 &_gen) override {
        log::WriteClientState("FastClientState::GenerateUpdateValueRequest");
        if (Cells_.Size_ < 2) {
            log::ForceWrite("FastClientState::GenerateUpdateValueRequest{SMALL SIZE} ", Cells_.Size_);
            std::exit(1);
        }
        std::uniform_int_distribution<uint32_t> idxDistrib(0, Cells_.Size_-2);
        auto node = GetNode(idxDistrib(_gen));
        auto &cell = node->Value_;
        std::uniform_int_distribution<model::Value> valueDistrib(0);
        model::Value value = valueDistrib(_gen);
        log::WriteClientState("FastClientState::GenerateUpdateValueRequest{id=", cell.CellId_, ", value=", value, '}');
        return api::UpdateValueRequest(cell.CellId_, value, Iteration_);
    }

    api::InsertValueRequest GenerateInsertValueRequest(std::mt19937 &_gen) override {
        log::WriteClientState("FastClientState::GenerateInsertValueRequest");
        if (Cells_.Size_ < 1) {
            log::ForceWrite("FastClientState::GenerateInsertValueRequest{SMALL SIZE} ", Cells_.Size_);
            std::exit(1);
        }
       std::uniform_int_distribution<uint32_t> idxDistrib(0, Cells_.Size_-1);
        model::CellId cellId = 0;
        if (uint32_t idx = idxDistrib(_gen)) {
            auto node = GetNode(idx - 1);
            cellId = node->Value_.CellId_;
        }
        std::uniform_int_distribution<model::Value> valueDistrib(0);
        model::Value value = valueDistrib(_gen);
        log::WriteClientState("FastClientState::GenerateInsertValueRequest{after=", cellId, ", value=", value, '}');
        return api::InsertValueRequest(cellId, value, Iteration_);
    }

    DecardTree<model::Cell>::Pointer GetNode(uint32_t idx)  {
        log::WriteClientState("FastClientState::GetNode{Cells_.Get start} ", idx, '/',  Cells_.Size_-1);
        auto node = Cells_.Get(idx);
        log::WriteClientState("FastClientState::GetNode{Cells_.Get ended}");
        if (node) {
            return node;
        } else {
            log::WriteClientState("FastClientState::GetNode{can't find node} ", idx, '/', Cells_.Size_-1);
            std::exit(1);
            return nullptr;
        }
    }

    api::DeleteValueRequest GenerateDeleteValueRequest(std::mt19937 &_gen) override {
        log::WriteClientState("FastClientState::GenerateDeleteValueRequest");
        if (Cells_.Size_ < 2) {
            log::ForceWrite("FastClientState::GenerateDeleteValueRequest{SMALL SIZE} ", Cells_.Size_);
            std::exit(1);
        }
        std::uniform_int_distribution<uint32_t> idxDistrib(0, Cells_.Size_-2);
        uint32_t idx = idxDistrib(_gen);
        log::WriteClientState("FastClientState::GenerateDeleteValueRequest{generated idx, find id}");
        auto node = GetNode(idx);
        model::CellId id = node->Value_.CellId_;
        log::WriteClientState("FastClientState::GenerateDeleteValueRequest{id=", id, '}');
        return api::DeleteValueRequest(id, Iteration_);
    }

    api::LoadStateRequest GenerateLoadStateRequest() override {
        log::WriteClientState("FastClientState::GenerateLoadState");
        return api::LoadStateRequest(Iteration_);
    }

    api::SyncRequest GenerateSyncRequest() override {
        log::WriteClientState("FastClientState::GenerateSyncRequest");
        return api::SyncRequest(Iteration_);
    }

    void ApplyOperations(const api::GenericResponse &_response) {
        std::unordered_map<model::CellId, const model::InsertValue*> postponedInserts;
        for (auto &update : _response.Updates_) {
            auto node = Nodes_[update.Cell_.CellId_];
            node->Value_.Value_ = update.Cell_.Value_;
        }
        for (auto &insert : _response.Insertions_) {
            uint32_t idx = 0;
            if (insert.NearCellId_) {
                auto node = Nodes_[insert.NearCellId_];
                if (!node) {
                    postponedInserts[insert.NearCellId_] = &insert;
                    continue;
                }
                idx = node->GetIdx() + 1;
            }

            std::unique_ptr<DecardTree<model::Cell>> newNode(new DecardTree<model::Cell>{.Priority_=GeneratePriority_(Generator_), .Value_=insert.Cell_});
            Nodes_[insert.Cell_.CellId_] = newNode.get();
            Cells_.Insert(idx, newNode.release());

            auto postponedIt = postponedInserts.find(insert.Cell_.CellId_);
            while (postponedIt != postponedInserts.end()) {
                auto node = Nodes_[postponedIt->second->NearCellId_];
                idx = node->GetIdx() + 1;

                std::unique_ptr<DecardTree<model::Cell>> newNode(new DecardTree<model::Cell>{.Priority_=GeneratePriority_(Generator_), .Value_=postponedIt->second->Cell_});
                Nodes_[postponedIt->second->Cell_.CellId_] = newNode.get();

                auto next = postponedInserts.find(postponedIt->second->Cell_.CellId_);
                postponedInserts.erase(std::exchange(postponedIt, next));
            }
        }
        for (auto &del : _response.Deletions_) {
            auto node = Nodes_[del.CellId_];
            uint32_t idx = node->GetIdx();
            auto [_, erased] =  Cells_.Erase(idx);
            delete erased;
        }
    }

    void HandleUpdateValueResponse(const api::UpdateValueResponse &_response) override {
        log::WriteClientState("FastClientState::HandleUpdateValueResponse ", _response.Iteration_);
        ApplyOperations(_response);
        Iteration_ = _response.Iteration_;
    }

    void HandleInsertValueResponse(const api::InsertValueResponse &_response) override {
        log::WriteClientState("FastClientState::HandleInsertValueResponse ", _response.Iteration_);
        ApplyOperations(_response);
        Iteration_ = _response.Iteration_;
    }

    void HandleDeleteValueResponse(const api::DeleteValueResponse &_response) override {
        log::WriteClientState("FastClientState::HandleDeleteValueResponse ", _response.Iteration_);
        ApplyOperations(_response);
        Iteration_ = _response.Iteration_;
    }

    void HandleSyncResponse(const api::SyncResponse &_response) override {
        log::WriteClientState("FastClientState::HandleSyncResponse");
        ApplyOperations(_response);
        Iteration_ = _response.Iteration_;
    }

    void HandleState(const api::State &_response) override {
        log::WriteClientState("FastClientState::HandleState");
        if (Cells_.Size_ == 1) {
            log::WriteClientState("FastClientState::HandleState{Init}");
            for (uint32_t idx = 0; idx < _response.Cells_.size(); ++idx) {
                log::WriteInit("Init idx=", idx, " id=", _response.Cells_[idx].CellId_);
                std::unique_ptr<DecardTree<model::Cell>> node(new DecardTree<model::Cell>{
                        .Priority_=GeneratePriority_(Generator_),
                        .Value_=_response.Cells_[idx]});
                auto raw = node.get();
                Nodes_[_response.Cells_[idx].CellId_] = node.get();
                Cells_.Insert(idx, node.release());
                if (magic_numbers::WithStateChecking && idx != raw->GetIdx()) {
                    log::WriteClientState("FastClientState::HandleState{IncorrectIdx} ", idx , ' ', raw->GetIdx());
                    return;
                }
            }
            log::WriteClientState("FastClientState::HandleState{Cells_.size()=", Cells_.Size_ - 1,'}');
            Iteration_ = _response.Iteration_;
            return;
        }
        if (magic_numbers::WithStateChecking) {
            ApplyOperations(_response);
            if (Cells_.Size_ - 1 != _response.Cells_.size()) {
                log::ForceWrite("Sizes aren't equal ", Cells_.Size_ - 1, ' ', _response.Cells_.size());
                for (uint32_t idx = 0; idx < Cells_.Size_ - 1; ++idx) {
                    auto &value = Cells_.Get(idx)->Value_;
                    log::WriteFullStateLog('<', idx, "> ", value.CellId_);
                }
                log::ForceWrite("END LIST");
                std::exit(1);
            }
            uint32_t idx = 0;
            for (auto &cell : _response.Cells_) {
                auto &value = Cells_.Get(idx)->Value_;
                if (value != cell) {
                    if (value.CellId_ !=  cell.CellId_) {
                        log::ForceWrite("Cells aren't equal at ", idx, ' ', value.CellId_, ' ', cell.CellId_);
                        for (uint32_t idx = 0; idx < Cells_.Size_ - 1; ++idx) {
                            auto &value = Cells_.Get(idx)->Value_;
                            log::WriteFullStateLog('<', idx, "> ", value.CellId_);
                        }
                        log::ForceWrite("END LIST");
                    } else {
                        log::ForceWrite("Cells equal by id at ", idx, ' ', value.CellId_, ' ', cell.CellId_);
                        log::ForceWrite("Cells aren't equal by value at ", idx, ' ', value.Value_, ' ', cell.Value_);
                    }
                    std::exit(1);
                }
                idx++;
            }
            Iteration_ = _response.Iteration_;
        }
    }
};

struct FastSmallClientState : IClientState {
    std::vector<model::CellId> CellIds_;
    std::unordered_set<model::CellId> DeletedIds_;

    model::IterationId Iteration_ = 0;

    FastSmallClientState()
    {}

    virtual ~FastSmallClientState(){}

    api::UpdateValueRequest GenerateUpdateValueRequest(std::mt19937 &_gen) override {
        std::uniform_int_distribution<uint32_t> idxDistrib(0, CellIds_.size()-1);
        uint32_t idx = idxDistrib(_gen);
        while (DeletedIds_.count(CellIds_[idx])) {
            idx = idxDistrib(_gen);
        }
        model::CellId cellId = CellIds_[idx];
        std::uniform_int_distribution<model::Value> valueDistrib(0);
        model::Value value = valueDistrib(_gen);
        log::WriteClientState("FastSmallClientState::GenerateUpdateValueRequest{id=", cellId, ", value=", value, '}');
        return api::UpdateValueRequest(cellId, value, Iteration_);
    }

    api::InsertValueRequest GenerateInsertValueRequest(std::mt19937 &_gen) override {
       std::uniform_int_distribution<uint32_t> idxDistrib(0, CellIds_.size());
        model::CellId cellId = 0;
        uint32_t idx = idxDistrib(_gen);
        while (idx && DeletedIds_.count(CellIds_[idx - 1])) {
            idx = idxDistrib(_gen);
        }
        if (idx) {
            cellId = CellIds_[idx - 1];
        }
        std::uniform_int_distribution<model::Value> valueDistrib(0);
        model::Value value = valueDistrib(_gen);
        log::WriteClientState("FastSmallClientState::GenerateInsertValueRequest{after=", cellId, ", value=", value, '}');
        return api::InsertValueRequest(cellId, value, Iteration_);
    }

    api::DeleteValueRequest GenerateDeleteValueRequest(std::mt19937 &_gen) override {
        std::uniform_int_distribution<uint32_t> idxDistrib(0, CellIds_.size()-1);
        uint32_t idx = idxDistrib(_gen);
        while (DeletedIds_.count(CellIds_[idx])) {
            idx = idxDistrib(_gen);
        }
        model::CellId id = CellIds_[idx];
        log::WriteClientState("FastSmallClientState::GenerateDeleteValueRequest{id=", id, '}');
        return api::DeleteValueRequest(id, Iteration_);
    }

    api::LoadStateRequest GenerateLoadStateRequest() override {
        log::WriteClientState("FastSmallClientState::GenerateLoadState");
        return api::LoadStateRequest(Iteration_);
    }

    api::SyncRequest GenerateSyncRequest() override {
        log::WriteClientState("FastSmallClientState::GenerateSyncRequest");
        return api::SyncRequest(Iteration_);
    }

    void ApplyOperations(const api::GenericResponse &response) {
        log::WriteClientState("FastSmallClientState::ApplyOperations");
        for (auto &insert : response.Insertions_) {
            CellIds_.push_back(insert.Cell_.CellId_);
        }
        for (auto &del : response.Deletions_) {
            DeletedIds_.insert(del.CellId_);
        }
    }

    void HandleUpdateValueResponse(const api::UpdateValueResponse &_response) override {
        log::WriteClientState("FastSmallClientState::HandleUpdateValueResponse ", _response.Iteration_);
        ApplyOperations(_response);
        Iteration_ = _response.Iteration_;
    }

    void HandleInsertValueResponse(const api::InsertValueResponse &_response) override {
        log::WriteClientState("FastSmallClientState::HandleInsertValueResponse ", _response.Iteration_);
        ApplyOperations(_response);
        Iteration_ = _response.Iteration_;
    }

    void HandleDeleteValueResponse(const api::DeleteValueResponse &_response) override {
        log::WriteClientState("FastSmallClientState::HandleDeleteValueResponse ", _response.Iteration_);
        ApplyOperations(_response);
        Iteration_ = _response.Iteration_;
    }

    void HandleSyncResponse(const api::SyncResponse &_response) override {
        log::WriteClientState("FastSmallClientState::HandleSyncResponse");
        ApplyOperations(_response);
        Iteration_ = _response.Iteration_;
    }

    void HandleState(const api::State &_response) override {
        log::WriteClientState("FastSmallClientState::HandleState");
        if (CellIds_.empty()) {
            log::WriteClientState("FastSmallClientState::HandleState{Init}");
            CellIds_.reserve(2 * _response.Cells_.size());
            for (uint32_t idx = 0; idx < _response.Cells_.size(); ++idx) {
                CellIds_.push_back(_response.Cells_[idx].CellId_);
            }
            log::WriteClientState("FastSmallClientState::HandleState{Cells_.size()=", CellIds_.size(),'}');
        }
    }
};

}
