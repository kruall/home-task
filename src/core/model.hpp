#pragma once

#include <cstdint>
#include <vector>
#include <deque>
#include <list>
#include <variant>


namespace home_task::model {

using CellId = uint64_t;
using Value = uint32_t;

struct Cell {
    CellId CellId_;
    Value Value_;

    Cell(CellId _cellId, Value _value)
        : CellId_(_cellId)
        , Value_(_value)
    {}

    bool operator==(const Cell &rhs) const {
        return CellId_ == rhs.CellId_ && Value_ == rhs.Value_;
    }
};

using CellList = std::list<model::Cell>;
using CellVector = std::vector<model::Cell>;

struct UpdateValue {
    Cell Cell_;
};

struct InsertValue {
    CellId NearCellId_;
    Cell Cell_;
};

struct DeleteValue {
    CellId CellId_;
};

using Operation = std::variant<UpdateValue, InsertValue, DeleteValue>;
using OperationDeq = std::deque<Operation>;

using IterationId = uint64_t;
using ClientId = uint32_t;

}
