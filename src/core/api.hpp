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
    SyncRequest,

    State = Begin + 1024,
    UpdateValueResponse,
    InsertValueResponse,
    DeleteValueResponse,
    SyncResponse,
};

struct UpdateValueRequest {
    static constexpr EAPIEventsType Type_ = EAPIEventsType::UpdateValueRequest;

    uint64_t PreviousIteration_ = 0;
    uint64_t CellId_;
    uint32_t Value_;

    UpdateValueRequest(uint64_t _cellId, uint32_t _value, uint64_t _iteration)
        : PreviousIteration_(_iteration)
        , CellId_(_cellId)
        , Value_(_value)
    {}
};

struct InsertValueRequest {
    static constexpr EAPIEventsType Type_ = EAPIEventsType::InsertValueRequest;

    uint64_t PreviousIteration_ = 0;
    uint64_t NearCellId_;
    uint32_t Value_;

    InsertValueRequest(uint64_t _cellId, uint32_t _value, uint64_t _iteration)
        : PreviousIteration_(_iteration)
        , NearCellId_(_cellId)
        , Value_(_value)
    {}
};

struct DeleteValueRequest {
    static constexpr EAPIEventsType Type_ = EAPIEventsType::DeleteValueRequest;

    uint64_t PreviousIteration_ = 0;
    uint64_t CellId_;

    DeleteValueRequest(uint64_t _cellId, uint64_t _iteration)
        : PreviousIteration_(_iteration)
        , CellId_(_cellId)
    {}
};

struct SyncRequest {
    static constexpr EAPIEventsType Type_ = EAPIEventsType::SyncRequest;

    uint64_t PreviousIteration_ = 0;

    SyncRequest(uint64_t _iteration)
        : PreviousIteration_(_iteration)
    {}
};

struct State {
    static constexpr EAPIEventsType Type_ = EAPIEventsType::State;

    std::vector<model::Cell> Cells_;
    uint64_t Iteration_ = 0;

    State(std::vector<model::Cell> &&_cells) : Cells_(std::move(_cells))
    {}

    uint32_t CalculateSize() const;
};


struct GenericResponse {
    struct UpdateValue {
        model::Cell Cell_;
    };
    struct InsertValue {
        uint64_t NearCellId_;
        model::Cell Cell_;
    };
    struct DeleteValue {
        uint64_t CellId_;
    };
    using Modification = std::variant<UpdateValue, InsertValue, DeleteValue>;

    std::vector<Modification> Modificatoins_;
    uint64_t Iteration_ = 0;

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

    uint64_t CellId_;

    InsertValueResponse(uint64_t _cellId)
        : CellId_(_cellId)
    {}

    uint32_t CalculateSize() const;
};

struct DeleteValueResponse : GenericResponse {
    static constexpr EAPIEventsType Type_ = EAPIEventsType::DeleteValueResponse;

    using GenericResponse::GenericResponse;
};

struct SyncResponse : GenericResponse {
    static constexpr EAPIEventsType Type_ = EAPIEventsType::SyncResponse;

    using GenericResponse::GenericResponse;
};

using MessageRecord = network_mock::MessageRecord;

inline std::unique_ptr<MessageRecord> MakeLoadStateMessage() {
    auto msg = std::make_unique<MessageRecord>();
    msg->Type_ = static_cast<uint32_t>(EAPIEventsType::LoadState);
    if constexpr (magic_numbers::WithSizeCalculation) {
        msg->Size_ = sizeof(MessageRecord);
    }
    return msg;
}

template <typename _Record, typename _Decay_t=std::decay_t<_Record>>
constexpr bool IsRequest = std::is_same_v<_Decay_t, UpdateValueRequest>
    || std::is_same_v<_Decay_t, InsertValueRequest>
    || std::is_same_v<_Decay_t, DeleteValueRequest>
    || std::is_same_v<_Decay_t, SyncRequest>;

template <typename _Record, typename _Decay_t=std::decay_t<_Record>>
constexpr bool IsResponse = std::is_same_v<_Decay_t, State>
    || std::is_same_v<_Decay_t, UpdateValueResponse>
    || std::is_same_v<_Decay_t, InsertValueResponse>
    || std::is_same_v<_Decay_t, DeleteValueResponse>
    || std::is_same_v<_Decay_t, SyncResponse>;

template <typename _Record, typename ... _Args>
inline std::unique_ptr<MessageRecord> MakeRequestMessage(_Args&& ... args) {
    static_assert(IsRequest<_Record>);
    auto msg = std::make_unique<MessageRecord>();
    msg->Type_ = static_cast<uint32_t>(_Record::Type_);
    msg->Record_ = std::make_any<_Record>(std::forward(args)...);
    if constexpr (magic_numbers::WithSizeCalculation) {
        msg->Size_ = sizeof(_Record) + sizeof(MessageRecord);
    }
    return msg;
}

template <typename _Record, typename _Decay_t=std::decay_t<_Record>>
inline std::unique_ptr<MessageRecord> MakeRequestMessage(_Record&& _record) {
    static_assert(IsRequest<_Record>);
    auto msg = std::make_unique<MessageRecord>();
    msg->Type_ = static_cast<uint32_t>(_Decay_t::Type_);
    msg->Record_ = std::make_any<_Decay_t>(std::move(_record));
    if constexpr (magic_numbers::WithSizeCalculation) {
        msg->Size_ = sizeof(_Decay_t) + sizeof(MessageRecord);
    }
    return msg;
}

template <typename _Record, typename _Decay_t=std::decay_t<_Record>>
inline std::unique_ptr<MessageRecord> MakeResponseMessage(_Record&& _record) {
    static_assert(IsResponse<_Record>);
    auto msg = std::make_unique<MessageRecord>();
    msg->Type_ = static_cast<uint32_t>(_Decay_t::Type_);
    msg->Record_ = std::make_any<_Decay_t>(std::move(_record));
    if constexpr (magic_numbers::WithSizeCalculation) {
        msg->Size_ = sizeof(_Decay_t) + std::any_cast<_Decay_t>(&msg->Record_)->CalculateSize() + sizeof(MessageRecord);
    }
    return msg;
}

}
