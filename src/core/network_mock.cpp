#include "network_mock.hpp"

using namespace home_task::network_mock;


NetworkMock::NetworkMock(uint32_t _mailBoxCount) {
    MailBoxes_.reserve(_mailBoxCount);
    for (uint32_t idx = 0; idx < _mailBoxCount; ++idx) {
        MailBoxes_.emplace_back(std::make_unique<MailBox>());
    }
}

void NetworkMock::Send(uint64_t _receiver, std::unique_ptr<MessageRecord> &&_msg) {
    std::lock_guard<std::mutex> guard(MailBoxes_[_receiver]->Mutex_);
    MailBoxes_[_receiver]->Queue_.push(std::move(_msg));
}

std::optional<std::unique_ptr<MessageRecord>> NetworkMock::Receive(uint64_t _mailbox) {
    std::lock_guard<std::mutex> guard(MailBoxes_[_mailbox]->Mutex_);
    auto &queue = MailBoxes_[_mailbox]->Queue_;
    if (queue.empty()) {
        return std::nullopt;
    }
    auto msg = std::move(queue.front());
    queue.pop();
    return msg;
}
