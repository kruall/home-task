#include <core/api.hpp>

using namespace home_task::api;


uint32_t State::CalculateSize() const {
    return sizeof(model::Cell) * Cells_.size();
}

uint32_t GenericResponse::CalculateSize() const {
    uint32_t acc = 0;
    auto getSize = [] (const auto &el) {
        return sizeof(decltype(el));
    };
    for (auto &el : Modificatoins_) {
        acc += std::visit(getSize, el);
    }
    return acc;
}

uint32_t InsertValueResponse::CalculateSize() const {
    return sizeof(CellId_) + GenericResponse::CalculateSize();
}
