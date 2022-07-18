#pragma once

#include "magic_numbers.hpp"
#include "log.hpp"

#include <any>
#include <cstdint>
#include <memory>
#include <optional>
#include <queue>
#include <mutex>
#include <string>
#include <condition_variable>

namespace home_task::network_mock {


enum class EMessageType : uint32_t {
    Ping,
    Pong,
    String,
    Poison,
    PRIVATE = 1024
};

struct MessageRecord {
    uint32_t Type_;
    std::any Record_;
    uint64_t Sender_;
    uint64_t Size_ = 0;

    ~MessageRecord() {
        log::Write("~MessageRecord");
    }
};


inline std::unique_ptr<MessageRecord> MakePingMessage() {
    auto msg = std::make_unique<MessageRecord>();
    msg->Type_ = static_cast<uint32_t>(home_task::network_mock::EMessageType::Ping);
    return msg;
}

inline std::unique_ptr<MessageRecord> MakePongMessage() {
    auto msg = std::make_unique<MessageRecord>();
    msg->Type_ = static_cast<uint32_t>(home_task::network_mock::EMessageType::Pong);
    return msg;
}

inline std::unique_ptr<MessageRecord> MakeStringMessage(const std::string &str) {
    auto msg = std::make_unique<MessageRecord>();
    msg->Type_ = (uint32_t)home_task::network_mock::EMessageType::String;
    msg->Record_ = str;
    if constexpr (magic_numbers::WithSizeCalculation) {
        msg->Size_ = str.size();
    }
    return msg;
}

inline std::unique_ptr<MessageRecord> MakePoisonMessage() {
    auto msg = std::make_unique<MessageRecord>();
    msg->Type_ = static_cast<uint32_t>(home_task::network_mock::EMessageType::Poison);
    return msg;
}

class NetworkMock {
    friend class NetworkClient;

    struct MailBox {
        std::queue<std::unique_ptr<MessageRecord>> Queue_;
        std::condition_variable Notifier_;
        std::mutex Mutex_;

        ~MailBox() {
            log::Write("~MailBox queue# ", Queue_.size());
        }
    };

    std::vector<std::unique_ptr<MailBox>> MailBoxes_;

public:
    NetworkMock(uint32_t _mailBoxCount);

    ~NetworkMock() {
        log::Write("~NetworkMock");
    }

    void Send(uint64_t _receiver, uint64_t _sender, std::unique_ptr<MessageRecord> &&_msg);

    std::optional<std::unique_ptr<MessageRecord>> Receive(uint64_t _mailbox);

    void WaitMessage(uint64_t _mailbox);

};

}
