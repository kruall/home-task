#include "network_mock.hpp"

using namespace home_task::network_mock;


NetworkMock::NetworkMock(uint32_t _mailBoxCount) {
    MailBoxes_.reserve(_mailBoxCount);
    for (uint32_t idx = 0; idx < _mailBoxCount; ++idx) {
        MailBoxes_.emplace_back(std::make_unique<MailBox>());
    }
}

void NetworkMock::Send(model::ClientId _receiver, model::ClientId _sender, std::unique_ptr<MessageRecord> &&_msg) {
    _msg->Sender_ = _sender;
    auto mailBox = MailBoxes_[_receiver].get();
    log::WriteMutex("NetworkMock::Send{lock queue mutex ", _receiver, '}');
    std::lock_guard<std::mutex> guard(mailBox->QueueMutex_);
    log::WriteMutex("NetworkMock::Send{locked queue mutex ", _receiver, '}');
    mailBox->Queue_.push(std::move(_msg));
    mailBox->Notifier_.notify_one();
    log::WriteMutex("NetworkMock::Send{unlock queue mutex ", _receiver, '}');
}

std::optional<std::unique_ptr<MessageRecord>> NetworkMock::Receive(model::ClientId _mailbox) {
    log::WriteMutex("NetworkMock::Receive{lock queue mutex ", _mailbox, '}');
    std::lock_guard<std::mutex> guard(MailBoxes_[_mailbox]->QueueMutex_);
    log::WriteMutex("NetworkMock::Receive{locked queue mutex ", _mailbox, '}');
    auto &queue = MailBoxes_[_mailbox]->Queue_;
    if (queue.empty()) {
        return std::nullopt;
    }
    auto msg = std::move(queue.front());
    queue.pop();
    log::WriteMutex("NetworkMock::Receive{unlock queue mutex ", _mailbox, '}');
    return msg;
}

std::unique_ptr<MessageRecord> NetworkMock::ReceiveWithWaiting(model::ClientId _mailbox) {
    auto msg = Receive(_mailbox);
    while (!msg) {
        WaitMessage(_mailbox);
        msg = Receive(_mailbox);
    }
    return std::move(*msg);
}

void NetworkMock::WaitMessage(model::ClientId _mailbox) {
    auto mailBox = MailBoxes_[_mailbox].get();
    log::WriteMutex("NetworkMock::Receive{lock notifier mutex ", _mailbox, '}');
    std::unique_lock<std::mutex> lock(mailBox->NotifierMutex_);
    log::WriteMutex("NetworkMock::Receive{locked notifier mutex ", _mailbox, '}');
    mailBox->Notifier_.wait(lock, [&]{ return mailBox->Queue_.size(); });
    log::WriteMutex("NetworkMock::Receive{unlock notifier mutex ", _mailbox, '}');
}

NetworkClient::NetworkClient(const std::shared_ptr<network_mock::NetworkMock> &_network, model::ClientId _mailBox)
    : Network_(_network)
    , MailBox_(_mailBox)
{}


void NetworkClient::Send(model::ClientId _receiver, std::unique_ptr<MessageRecord> &&_msg) {
    log::WriteNetwork("NetworkClient::Send ", MailBox_, "->", _receiver);
    Network_->Send(_receiver, MailBox_, std::move(_msg));
}

std::optional<std::unique_ptr<MessageRecord>> NetworkClient::Receive() {
    log::WriteNetwork("NetworkClient::Receive ", MailBox_);
    return Network_->Receive(MailBox_);
}

std::unique_ptr<MessageRecord> NetworkClient::ReceiveWithWaiting() {
    log::WriteNetwork("NetworkClient::ReceiveWithWaiting ", MailBox_);
    return Network_->ReceiveWithWaiting(MailBox_);
}

void NetworkClient::WaitMessage() {
    log::WriteNetwork("NetworkClient::WaitMessage ", MailBox_);
    Network_->WaitMessage(MailBox_);
}
