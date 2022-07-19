#pragma once

#include <core/api.hpp>

#include <algorithm>
#include <random>
#include <deque>
#include <cassert>
#include <unordered_set>


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
        log::WriteClientState("ClientStateNop::GenerateUpdateValueRequest");
        return api::UpdateValueRequest(0, 0, 0);
    }

    virtual api::InsertValueRequest GenerateInsertValueRequest(std::mt19937 &) {
        log::WriteClientState("ClientStateNop::GenerateInsertValueRequest");
        return api::InsertValueRequest(0, 0, 0);
    }

    virtual api::DeleteValueRequest GenerateDeleteValueRequest(std::mt19937 &) {
        log::WriteClientState("ClientStateNop::GenerateDeleteValueRequest");
        return api::DeleteValueRequest(0, 0);
    }

    virtual void HandleUpdateValueResponse(const api::UpdateValueResponse &) {
        log::WriteClientState("ClientStateNop::HandleUpdateValueResponse");
    }

    virtual void HandleInsertValueResponse(const api::InsertValueResponse &) {
        log::WriteClientState("ClientStateNop::HandleInsertValueResponse");
    }

    virtual void HandleDeleteValueResponse(const api::DeleteValueResponse &) {
        log::WriteClientState("ClientStateNop::HandleDeleteValueResponse");
    }

    virtual void HandleState(const api::State &) {
        log::WriteClientState("ClientStateNop::HandleState");
    }
};



struct ClientState : IClientState {
    std::deque<model::Cell> Cells_;
    uint64_t Iteration_ = 0;

    virtual ~ClientState(){}

    api::UpdateValueRequest GenerateUpdateValueRequest(std::mt19937 &_gen) override {
        std::uniform_int_distribution<> idxDistrib(0, Cells_.size()-1);
        auto &cell = Cells_[idxDistrib(_gen)];
        std::uniform_int_distribution<uint32_t> valueDistrib(0);
        uint32_t value = valueDistrib(_gen);
        log::WriteClientState("ClientState::GenerateUpdateValueRequest{id=", cell.CellId_, ", value=", value, '}');
        return api::UpdateValueRequest(cell.CellId_, value, Iteration_);
    }

    api::InsertValueRequest GenerateInsertValueRequest(std::mt19937 &_gen) override {
        std::uniform_int_distribution<> idxDistrib(0, Cells_.size());
        uint64_t cellId = 0;
        if (uint32_t idx = idxDistrib(_gen)) {
            cellId = Cells_[idx - 1].CellId_;
        }
        std::uniform_int_distribution<> valueDistrib(0);
        uint32_t value = valueDistrib(_gen);
        log::WriteClientState("ClientState::GenerateInsertValueRequest{after=", cellId, ", value=", value, '}');
        return api::InsertValueRequest(cellId, value, Iteration_);
    }

    api::DeleteValueRequest GenerateDeleteValueRequest(std::mt19937 &_gen) override {
        std::uniform_int_distribution<> idxDistrib(0, Cells_.size()-1);
        uint64_t id = idxDistrib(_gen);
        log::WriteClientState("ClientState::GenerateDeleteValueRequest{id=", id, '}');
        return api::DeleteValueRequest(Cells_[id].CellId_, Iteration_);
    }

    void Update(const std::vector<api::GenericResponse::Modification> &_modifications) {
        auto update = [&] (auto cmd) {
            using type = std::decay_t<decltype(cmd)>;
            if constexpr (std::is_same_v<type, api::GenericResponse::UpdateValue>) {
                auto it = std::find_if(Cells_.begin(), Cells_.end(), [id=cmd.Cell_.CellId_] (auto &el) { return el.CellId_ == id;});
                it->Value_ = cmd.Cell_.Value_;
            }
            if constexpr (std::is_same_v<type, api::GenericResponse::InsertValue>) {
                auto it = std::find_if(Cells_.begin(), Cells_.end(), [id=cmd.NearCellId_] (auto &el) { return el.CellId_ == id;});
                it++;
                Cells_.insert(it, cmd.Cell_);
            }
            if constexpr (std::is_same_v<type, api::GenericResponse::DeleteValue>) {
                auto it = std::find_if(Cells_.begin(), Cells_.end(), [id=cmd.CellId_] (auto &el) { return el.CellId_ == id;});
                Cells_.erase(it);
            }
        };
        for (auto &el : _modifications) {
            std::visit(update, el);
        }
    }

    void HandleUpdateValueResponse(const api::UpdateValueResponse &_response) override {
        log::WriteClientState("ClientState::HandleUpdateValueResponse");
        Update(_response.Modificatoins_);
        Iteration_ = _response.Iteration_;
    }

    void HandleInsertValueResponse(const api::InsertValueResponse &_response) override {
        log::WriteClientState("ClientState::HandleInsertValueResponse");
        Update(_response.Modificatoins_);
        Iteration_ = _response.Iteration_;
    }

    void HandleDeleteValueResponse(const api::DeleteValueResponse &_response) override {
        log::WriteClientState("ClientState::HandleDeleteValueResponse");
        Update(_response.Modificatoins_);
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
            assert(Cells_.size() == _response.Cells_.size());
            uint32_t idx = 0;
            for (auto &cell : Cells_) {
                assert(cell == _response.Cells_[idx]);
                idx++;
            }
        }
    }
};

template <typename _Value>
struct DecardTree {
    using Pointer = DecardTree*;
    using ConstPointer = const DecardTree*;

    Pointer Left_ = nullptr;
    Pointer Right_ = nullptr;
    Pointer Parent_ = nullptr;

    uint64_t Size_ = 1;
    uint64_t Priority_ = 0;
    
    _Value Value_;

    ~DecardTree() {
        if (Left_) {
            delete Left_;
        }
        if (Right_) {
            delete Right_;
        }
    }

    uint64_t GetLeftSize() const {
        if (Left_) {
            return Left_->Size_;
        }
        return 0;
    }

    uint64_t GetRightSize() const {
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

    std::pair<Pointer, Pointer> SplitByIndex(uint64_t _idx) {
        log::WriteDecardTree("SplitByIndex ", _idx);
        if (_idx > Size_) {
            log::WriteDecardTree("SplitByIndex{TooBig} ", _idx, '/', Size_);
        }
        if (_idx == Size_) {
            log::WriteDecardTree("SplitByIndex{Big} ", _idx);
            Parent_ = nullptr;
            Size_ = 1 + GetLeftSize() + GetRightSize();
            return {this, nullptr};
        }
        uint64_t leftSize = GetLeftSize();
        if (leftSize < _idx) {
            log::WriteDecardTree("SplitByIndex{Right} ", _idx);
            if (Right_) {
                auto [left, right] = UntieRight()->SplitByIndex(_idx - leftSize - 1);
                Right_ = left;
                TieRight();
                Parent_ = nullptr;
                if (right) right->Parent_ = nullptr;
                Size_ = 1 + GetLeftSize() + GetRightSize();
                if (right) right->Size_ = 1 + right->GetLeftSize() + right->GetRightSize();
                return {this, right};
            } else {
                Parent_ = nullptr;
                Size_ = 1 + GetLeftSize() + GetRightSize();
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
                Size_ = 1 + GetLeftSize() + GetRightSize();
                if (left) left->Size_ = 1 + left->GetLeftSize() + left->GetRightSize();;
                return {left, this};
            } else {
                Parent_ = nullptr;
                Size_ = 1 + GetLeftSize() + GetRightSize();
                return {nullptr, this};
            }
        }
    }

    Pointer Insert(uint64_t _idx, Pointer _node) {
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
            _node->Size_ = 1 + _node->GetLeftSize() + _node->GetRightSize();
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
        Size_ = 1 + GetLeftSize() + GetRightSize();
        Parent_ = nullptr;
        return this;
    }

    Pointer Merge(Pointer _tree) {
        log::WriteDecardTree("Merge");
        if (!_tree) {
            Parent_ = nullptr;
            return this;
        }
        if (Priority_ > _tree->Priority_) {
            if (Right_) {
                Right_ = Right_->Merge(_tree);
            } else {
                Right_ = _tree;
            }
            TieRight();
            Parent_ = nullptr;
            Size_ = 1 + GetLeftSize() + GetRightSize();
            return this;
        } else {
            if (_tree->Left_) {
                _tree->Left_ = _tree->Left_->Merge(this);
            } else {
                _tree->Left_ = this;
            }
            _tree->TieLeft();
            _tree->Parent_ = nullptr;
            _tree->Size_ = 1 + _tree->GetLeftSize() + _tree->GetRightSize();
            return _tree;
        }
    }

    uint64_t GetIdx() const {
        ConstPointer current = this;
        uint64_t acc = GetLeftSize();
        while (current->Parent_) {
            if (current->Parent_->Right_ == current) {
                acc += current->Parent_->GetLeftSize() + 1;
            }
            current = current->Parent_;
        }
        return acc;
    }

    Pointer Get(uint64_t _idx) {
        uint64_t leftSize = GetLeftSize();
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

    std::pair<Pointer, Pointer> Erase(uint64_t _idx) {
        uint64_t leftSize = GetLeftSize();
        if (leftSize == _idx) {
            auto res = Left_ ? Left_->Merge(Right_) : Right_;
            Left_ = nullptr;
            Right_ = nullptr;
            Parent_ = nullptr;
            Size_ = 1;
            return {res, this};
        } else {
            if (leftSize > _idx) {
                auto [left, erased] = Left_->Erase(_idx);
                Left_ = left;
                TieLeft();
                Size_--;
                return {this, erased};
            } else {
                auto [right, erased] = Right_->Erase(_idx - leftSize - 1);
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
    std::unordered_map<uint64_t, DecardTree<model::Cell>::Pointer> Nodes_;

    std::random_device RandomDevice_;
    std::mt19937 Generator_;
    std::uniform_int_distribution<uint64_t> GeneratePriority_;

    uint64_t Iteration_ = 0;

    FastClientState()
        : Cells_(DecardTree<model::Cell>{.Priority_ = std::numeric_limits<uint64_t>::max(), .Value_={0, 0}})
        , Generator_(RandomDevice_())
        , GeneratePriority_(0, std::numeric_limits<uint64_t>::max() - 1)
    {}

    virtual ~FastClientState(){}

    api::UpdateValueRequest GenerateUpdateValueRequest(std::mt19937 &_gen) override {
        std::uniform_int_distribution<> idxDistrib(0, Cells_.Size_-2);
        auto &cell = Cells_.Get(idxDistrib(_gen))->Value_;
        std::uniform_int_distribution<uint32_t> valueDistrib(0);
        uint32_t value = valueDistrib(_gen);
        log::WriteClientState("FastClientState::GenerateUpdateValueRequest{id=", cell.CellId_, ", value=", value, '}');
        return api::UpdateValueRequest(cell.CellId_, value, Iteration_);
    }

    api::InsertValueRequest GenerateInsertValueRequest(std::mt19937 &_gen) override {
       std::uniform_int_distribution<> idxDistrib(0, Cells_.Size_-1);
        uint64_t cellId = 0;
        if (uint32_t idx = idxDistrib(_gen)) {
            cellId = Cells_.Get(idx - 1)->Value_.CellId_;
        }
        std::uniform_int_distribution<uint32_t> valueDistrib(0);
        uint32_t value = valueDistrib(_gen);
        log::WriteClientState("FastClientState::GenerateInsertValueRequest{after=", cellId, ", value=", value, '}');
        return api::InsertValueRequest(cellId, value, Iteration_);
    }

    api::DeleteValueRequest GenerateDeleteValueRequest(std::mt19937 &_gen) override {
        std::uniform_int_distribution<> idxDistrib(0, Cells_.Size_-2);
        uint64_t idx = idxDistrib(_gen);
        uint64_t id = Cells_.Get(idx - 1)->Value_.CellId_;
        log::WriteClientState("FastClientState::GenerateDeleteValueRequest{id=", id, '}');
        return api::DeleteValueRequest(id, Iteration_);
    }

    void Update(const std::vector<api::GenericResponse::Modification> &_modifications) {
        log::WriteClientState("FastClientState::Update ", _modifications.size());
        auto update = [&] (auto cmd) {
            using type = std::decay_t<decltype(cmd)>;
            if constexpr (std::is_same_v<type, api::GenericResponse::UpdateValue>) {
                log::WriteClientState("FastClientState::Update{UpdateValue}");
                auto node = Nodes_[cmd.Cell_.CellId_];
                if (!node) {
                    log::WriteFastClientState("can't find node ", cmd.Cell_.CellId_);
                }
                node->Value_.Value_ = cmd.Cell_.Value_;
            }
            if constexpr (std::is_same_v<type, api::GenericResponse::InsertValue>) {
                log::WriteClientState("FastClientState::Update{InsertValue}");
                uint64_t idx = 0;
                if (cmd.NearCellId_) {
                    auto node = Nodes_[cmd.NearCellId_];
                    if (!node) {
                        log::WriteFastClientState("can't find node ", cmd.NearCellId_);
                    }
                    idx = node->GetIdx() + 1;
                }
                std::unique_ptr<DecardTree<model::Cell>> newNode(new DecardTree<model::Cell>{.Priority_=GeneratePriority_(Generator_), .Value_=cmd.Cell_});
                Nodes_[cmd.Cell_.CellId_] = newNode.get();
                log::WriteFastClientState("before decard");
                Cells_.Insert(idx, newNode.release());
                log::WriteFastClientState("after decard");
            }
            if constexpr (std::is_same_v<type, api::GenericResponse::DeleteValue>) {
                log::WriteClientState("FastClientState::Update{DeleteValue}");
                auto node = Nodes_[cmd.CellId_];
                if (!node) {
                    log::WriteFastClientState("can't find node ", cmd.CellId_);
                }
                uint64_t idx = node->GetIdx();
                log::WriteFastClientState("idx for erasing ", idx, '/', Cells_.Size_ - 1);
                Nodes_.erase(cmd.CellId_);
                auto [root, erased] = Cells_.Erase(idx);
                if (erased) {
                    delete erased;
                }
                else {
                    log::WriteFastClientState("can't find node for erasing ", cmd.CellId_);
                }
            }
        };
        for (auto &el : _modifications) {
            std::visit(update, el);
        }
    }

    void HandleUpdateValueResponse(const api::UpdateValueResponse &_response) override {
        log::WriteClientState("FastClientState::HandleUpdateValueResponse ", _response.Iteration_, ' ', _response.Modificatoins_.size());
        Update(_response.Modificatoins_);
        Iteration_ = _response.Iteration_;
    }

    void HandleInsertValueResponse(const api::InsertValueResponse &_response) override {
        log::WriteClientState("FastClientState::HandleInsertValueResponse ", _response.Iteration_, ' ', _response.Modificatoins_.size());
        Update(_response.Modificatoins_);
        Iteration_ = _response.Iteration_;
    }

    void HandleDeleteValueResponse(const api::DeleteValueResponse &_response) override {
        log::WriteClientState("FastClientState::HandleDeleteValueResponse ", _response.Iteration_, ' ', _response.Modificatoins_.size());
        Update(_response.Modificatoins_);
        Iteration_ = _response.Iteration_;
    }

    void HandleState(const api::State &_response) override {
        log::WriteClientState("FastClientState::HandleState");
        if (Cells_.Size_ == 1) {
            log::WriteClientState("FastClientState::HandleState{Init}");
            for (uint64_t idx = 0; idx < _response.Cells_.size(); ++idx) {
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
        }
    }
};




struct FastSmallClientState : IClientState {
    std::vector<uint64_t> CellIds_;
    std::unordered_set<uint64_t> DeletedIds_;

    std::random_device RandomDevice_;
    std::mt19937 Generator_;
    std::uniform_int_distribution<uint64_t> GeneratePriority_;

    uint64_t Iteration_ = 0;

    FastSmallClientState()
        : Generator_(RandomDevice_())
        , GeneratePriority_(0, std::numeric_limits<uint64_t>::max() - 1)
    {}

    virtual ~FastSmallClientState(){}

    api::UpdateValueRequest GenerateUpdateValueRequest(std::mt19937 &_gen) override {
        std::uniform_int_distribution<uint32_t> idxDistrib(0, CellIds_.size()-1);
        uint32_t idx = idxDistrib(_gen);
        while (DeletedIds_.count(CellIds_[idx])) {
            idx = idxDistrib(_gen);
        }
        uint64_t cellId = CellIds_[idx];
        std::uniform_int_distribution<uint32_t> valueDistrib(0);
        uint32_t value = valueDistrib(_gen);
        log::WriteClientState("FastSmallClientState::GenerateUpdateValueRequest{id=", cellId, ", value=", value, '}');
        return api::UpdateValueRequest(cellId, value, Iteration_);
    }

    api::InsertValueRequest GenerateInsertValueRequest(std::mt19937 &_gen) override {
       std::uniform_int_distribution<> idxDistrib(0, CellIds_.size());
        uint64_t cellId = 0;
        uint32_t idx = idxDistrib(_gen);
        while (idx && DeletedIds_.count(CellIds_[idx - 1])) {
            idx = idxDistrib(_gen);
        }
        if (idx) {
            cellId = CellIds_[idx - 1];
        }
        std::uniform_int_distribution<uint32_t> valueDistrib(0);
        uint32_t value = valueDistrib(_gen);
        log::WriteClientState("FastSmallClientState::GenerateInsertValueRequest{after=", cellId, ", value=", value, '}');
        return api::InsertValueRequest(cellId, value, Iteration_);
    }

    api::DeleteValueRequest GenerateDeleteValueRequest(std::mt19937 &_gen) override {
        std::uniform_int_distribution<uint32_t> idxDistrib(0, CellIds_.size()-1);
        uint64_t idx = idxDistrib(_gen);
        while (DeletedIds_.count(CellIds_[idx])) {
            idx = idxDistrib(_gen);
        }
        uint64_t id = CellIds_[idx];
        log::WriteClientState("FastSmallClientState::GenerateDeleteValueRequest{id=", id, '}');
        return api::DeleteValueRequest(id, Iteration_);
    }

    void Update(const std::vector<api::GenericResponse::Modification> &_modifications) {
        log::WriteClientState("FastSmallClientState::Update ", _modifications.size());
        auto update = [&] (auto cmd) {
            using type = std::decay_t<decltype(cmd)>;
            if constexpr (std::is_same_v<type, api::GenericResponse::InsertValue>) {
                log::WriteClientState("FastSmallClientState::Update{InsertValue ", cmd.Cell_.CellId_,"}");
                CellIds_.push_back(cmd.Cell_.CellId_);
            }
            if constexpr (std::is_same_v<type, api::GenericResponse::DeleteValue>) {
                log::WriteClientState("FastSmallClientState::Update{DeleteValue ", cmd.CellId_,"}");
                DeletedIds_.insert(cmd.CellId_);
            }
        };
        for (auto &el : _modifications) {
            std::visit(update, el);
        }
    }

    void HandleUpdateValueResponse(const api::UpdateValueResponse &_response) override {
        log::WriteClientState("FastSmallClientState::HandleUpdateValueResponse ", _response.Iteration_, ' ', _response.Modificatoins_.size());
        Update(_response.Modificatoins_);
        Iteration_ = _response.Iteration_;
    }

    void HandleInsertValueResponse(const api::InsertValueResponse &_response) override {
        log::WriteClientState("FastSmallClientState::HandleInsertValueResponse ", _response.Iteration_, ' ', _response.Modificatoins_.size());
        Update(_response.Modificatoins_);
        Iteration_ = _response.Iteration_;
    }

    void HandleDeleteValueResponse(const api::DeleteValueResponse &_response) override {
        log::WriteClientState("FastSmallClientState::HandleDeleteValueResponse ", _response.Iteration_, ' ', _response.Modificatoins_.size());
        Update(_response.Modificatoins_);
        Iteration_ = _response.Iteration_;
    }

    void HandleState(const api::State &_response) override {
        log::WriteClientState("FastSmallClientState::HandleState");
        if (CellIds_.empty()) {
            log::WriteClientState("FastSmallClientState::HandleState{Init}");
            CellIds_.reserve(2 * _response.Cells_.size());
            for (uint64_t idx = 0; idx < _response.Cells_.size(); ++idx) {
                CellIds_.push_back(_response.Cells_[idx].CellId_);
            }
            log::WriteClientState("FastSmallClientState::HandleState{Cells_.size()=", CellIds_.size(),'}');
        }
    }
};


};