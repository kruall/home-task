#include <core/api.hpp>

using namespace home_task::api;


uint32_t State::CalculateSize() const {
    return sizeof(model::Cell) * Cells_.size();
}

uint32_t GenericResponse::CalculateSize() const {
    uint32_t acc = 0;
    acc += Insertions_.size() * sizeof(model::InsertValue);
    acc += Updates_.size() * sizeof(model::UpdateValue);
    acc += Deletions_.size() * sizeof(model::DeleteValue);
    return acc;
}

uint32_t InsertValueResponse::CalculateSize() const {
    return sizeof(CellId_) + GenericResponse::CalculateSize();
}
