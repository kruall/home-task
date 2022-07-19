#pragma once


namespace home_task::model {

struct Cell {
    uint64_t CellId_;
    uint32_t Value_;

    Cell(uint64_t _cellId, uint32_t _value)
        : CellId_(_cellId)
        , Value_(_value)
    {}

    bool operator==(const Cell &rhs) const {
        return CellId_ == rhs.CellId_ && Value_ == rhs.Value_;
    }
};

}
