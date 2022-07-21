#pragma once

#include <core/network_mock.hpp>
#include <core/magic_numbers.hpp>
#include <core/model.hpp>

#include <vector>
#include <variant>


namespace home_task::api {

enum class EAPIEventsType : uint32_t {
    Begin = static_cast<uint32_t>(network_mock::EMessageType::PRIVATE),
    LoadStateRequest = Begin,
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


struct LoadStateRequest {
    static constexpr EAPIEventsType Type_ = EAPIEventsType::LoadStateRequest;

    model::IterationId PreviousIteration_ = 0;

    LoadStateRequest(model::IterationId _iteration)
        : PreviousIteration_(_iteration)
    {}
};

struct UpdateValueRequest {
    static constexpr EAPIEventsType Type_ = EAPIEventsType::UpdateValueRequest;

    model::IterationId PreviousIteration_ = 0;
    model::CellId CellId_;
    model::Value Value_;

    UpdateValueRequest(model::CellId _cellId, model::Value _value, model::IterationId _iteration)
        : PreviousIteration_(_iteration)
        , CellId_(_cellId)
        , Value_(_value)
    {}
};

struct InsertValueRequest {
    static constexpr EAPIEventsType Type_ = EAPIEventsType::InsertValueRequest;

    model::IterationId PreviousIteration_ = 0;
    model::CellId NearCellId_;
    model::Value Value_;

    InsertValueRequest(model::CellId _cellId, model::Value _value, model::IterationId _iteration)
        : PreviousIteration_(_iteration)
        , NearCellId_(_cellId)
        , Value_(_value)
    {}
};

struct DeleteValueRequest {
    static constexpr EAPIEventsType Type_ = EAPIEventsType::DeleteValueRequest;

    model::IterationId PreviousIteration_ = 0;
    model::CellId CellId_;

    DeleteValueRequest(model::CellId _cellId, model::IterationId _iteration)
        : PreviousIteration_(_iteration)
        , CellId_(_cellId)
    {}
};

struct SyncRequest {
    static constexpr EAPIEventsType Type_ = EAPIEventsType::SyncRequest;

    model::IterationId PreviousIteration_ = 0;

    SyncRequest(model::IterationId _iteration)
        : PreviousIteration_(_iteration)
    {}
};

struct GenericResponse {
    std::deque<model::UpdateValue> Updates_;
    std::deque<model::InsertValue> Insertions_;
    std::deque<model::DeleteValue> Deletions_;
    model::IterationId Iteration_ = 0;

    GenericResponse() = default;

    uint32_t CalculateSize() const;
};

struct State : GenericResponse {
    static constexpr EAPIEventsType Type_ = EAPIEventsType::State;

    std::vector<model::Cell> Cells_;
    model::IterationId Iteration_ = 0;

    State(std::vector<model::Cell> &&_cells) : Cells_(std::move(_cells))
    {}

    uint32_t CalculateSize() const;
};

struct UpdateValueResponse : GenericResponse {
    static constexpr EAPIEventsType Type_ = EAPIEventsType::UpdateValueResponse;

    using GenericResponse::GenericResponse;
};

struct InsertValueResponse : GenericResponse {
    static constexpr EAPIEventsType Type_ = EAPIEventsType::InsertValueResponse;

    model::CellId CellId_;

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


template <typename _Record, typename _Decay_t=std::decay_t<_Record>>
constexpr bool IsRequest = std::is_same_v<_Decay_t, LoadStateRequest>
    || std::is_same_v<_Decay_t, UpdateValueRequest>
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
