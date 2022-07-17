#pragma once


namespace home_task::model {

struct Cell {
    uint32_t CellId_;
    uint32_t Value_;

    Cell(uint32_t _cellId, uint32_t _value)
        : CellId_(_cellId)
        , Value_(_value)
    {}
};

}
