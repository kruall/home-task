#pragma once

#include <core/network_mock.hpp>
#include <core/magic_numbers.hpp>
#include <core/model.hpp>

#include <vector>
#include <variant>


namespace home_task::api {

enum class EAPIEventsType : uint32_t {
    Begin = static_cast<uint32_t>(network_mock::EMessageType::PRIVATE),
    LoadState = Begin,
    UpdateValueRequest,
    InsertValueRequest,
    DeleteValueRequest,

    State = Begin + 1024,
    UpdateValueResponse,
    InsertValueResponse,
    DeleteValueResponse,
};

struct UpdateValueRequest {
    static constexpr EAPIEventsType Type_ = EAPIEventsType::UpdateValueRequest;

    uint32_t CellId_;
    uint32_t Value_;

    UpdateValueRequest(uint32_t _cellId, uint32_t _value)
        : CellId_(_cellId)
        , Value_(_value)
    {}
};

struct InsertValueRequest {
    static constexpr EAPIEventsType Type_ = EAPIEventsType::InsertValueRequest;

    uint32_t NearCellId_;
    uint32_t Value_;

    InsertValueRequest(uint32_t _cellId, uint32_t _value)
        : NearCellId_(_cellId)
        , Value_(_value)
    {}
};

struct DeleteValueRequest {
    static constexpr EAPIEventsType Type_ = EAPIEventsType::DeleteValueRequest;

    uint32_t CellId_;

    DeleteValueRequest(uint32_t _cellId) : CellId_(_cellId)
    {}
};

struct State {
    static constexpr EAPIEventsType Type_ = EAPIEventsType::State;

    std::vector<model::Cell> Cells_;

    State(std::vector<model::Cell> &&_cells) : Cells_(_cells)
    {}

    uint32_t CalculateSize() const;
};


struct GenericResponse {
    struct UpdateValue {
        model::Cell Cell_;
    };
    struct InsertValue {
        uint32_t NearCellId_;
        model::Cell Cell_;
    };
    struct DeleteValue {
        uint32_t CellId_;
    };
    using Modification = std::variant<UpdateValue, InsertValue, DeleteValue>;

    std::vector<Modification> Modificatoins_;

    GenericResponse() = default;
    GenericResponse(std::vector<Modification> &&_modifications) : Modificatoins_(_modifications)
    {}


    uint32_t CalculateSize() const;
};

struct UpdateValueResponse : GenericResponse {
    static constexpr EAPIEventsType Type_ = EAPIEventsType::UpdateValueResponse;

    using GenericResponse::GenericResponse;
};

struct InsertValueResponse : GenericResponse {
    static constexpr EAPIEventsType Type_ = EAPIEventsType::InsertValueResponse;

    uint32_t CellId_;

    InsertValueResponse(uint32_t _cellId, std::vector<Modification> &&_modifications)
        : GenericResponse(std::move(_modifications))
        , CellId_(_cellId)
    {}

    uint32_t CalculateSize() const;
};

struct DeleteValueResponse : GenericResponse {
    static constexpr EAPIEventsType Type_ = EAPIEventsType::DeleteValueResponse;

    using GenericResponse::GenericResponse;
};

using MessageRecord = network_mock::MessageRecord;

inline std::unique_ptr<MessageRecord> MakeLoadStateMessage() {
    auto msg = std::make_unique<MessageRecord>();
    msg->Type_ = static_cast<uint32_t>(EAPIEventsType::LoadState);
    return msg;
}

template <typename _Record>
constexpr bool IsRequest = std::is_same_v<_Record, UpdateValueRequest>
    || std::is_same_v<_Record, InsertValueRequest>
    || std::is_same_v<_Record, DeleteValueRequest>;

template <typename _Record>
constexpr bool IsResponse = std::is_same_v<_Record, State>
    || std::is_same_v<_Record, UpdateValueResponse>
    || std::is_same_v<_Record, InsertValueResponse>
    || std::is_same_v<_Record, DeleteValueResponse>;

template <typename _Record, typename ... _Args>
inline std::unique_ptr<MessageRecord> MakeRequestMessage(_Args&& ... args) {
    static_assert(IsRequest<_Record>);
    auto msg = std::make_unique<MessageRecord>();
    msg->Type_ = static_cast<uint32_t>(_Record::Type_);
    msg->Record_ = std::make_any<_Record>(std::forward(args)...);
    if constexpr (magic_numbers::WithSizeCalculation) {
        msg->Size_ = sizeof(_Record);
    }
    return msg;
}

template <typename _Record>
inline std::unique_ptr<MessageRecord> MakeRequestMessage(_Record&& _record) {
    static_assert(IsRequest<_Record>);
    auto msg = std::make_unique<MessageRecord>();
    msg->Type_ = static_cast<uint32_t>(_Record::Type_);
    msg->Record_ = std::make_any<_Record>(std::move(_record));
    if constexpr (magic_numbers::WithSizeCalculation) {
        msg->Size_ = sizeof(_Record);
    }
    return msg;
}

template <typename _Record>
inline std::unique_ptr<MessageRecord> MakeResponseMessage(_Record&& _record) {
    static_assert(IsResponse<_Record>);
    auto msg = std::make_unique<MessageRecord>();
    msg->Type_ = static_cast<uint32_t>(_Record::Type_);
    msg->Record_ = std::make_any<_Record>(std::move(_record));
    if constexpr (magic_numbers::WithSizeCalculation) {
        msg->Size_ = std::any_cast<_Record>(&msg->Record_)->CalculateSize();
    }
    return msg;
}

}
