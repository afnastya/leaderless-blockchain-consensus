#pragma once

#include <chrono>
#include <glog/logging.h>
#include <boost/asio.hpp>
#include <boost/fiber/buffered_channel.hpp>
#include <optional>
#include <random>

#include "../core/message.hpp"

class Sender;

class IChannel {
public:
    virtual void set_handler(std::function<void(Message)>) = 0;
    virtual Sender get_sender() = 0;
    virtual void close() = 0;
    virtual void send(Message) = 0;
    virtual std::optional<Message> receive() = 0;
    virtual void handle_messages() = 0;
    virtual void stop() = 0;
    virtual ~IChannel() = default;

public:
    constexpr static auto default_handler = [](Message) { return; };

    std::function<void(Message)> handler_{default_handler};

    std::atomic<bool> is_stopped_{false};
    std::atomic<uint32_t> size_{0};
};


class Sender {
public:
    Sender(IChannel& channel) : channel_(channel) {
    }

    void send(Message msg) {
        channel_.send(std::move(msg));
    }

private:
    IChannel& channel_;
};

class Channel : public IChannel {
public:
    // size should be the power of 2
    Channel() : channel_(SIZE) {
    }

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    Channel(Channel&&) = delete;
    Channel& operator=(Channel&&) = delete;

    void set_handler(std::function<void(Message)> handler) override {
        handler_ = handler;
    }

    Sender get_sender() override {
        return Sender(*this);
    }

    uint32_t size() {
        return size_;
    }

    void close() override {
        stop();
        channel_.close();
    }

    void send(Message msg) override {
        if (is_stopped_) {
            return;
        }

        if (channel_.try_push(std::move(msg)) == boost::fibers::channel_op_status::full) {
            LOG(FATAL) << "channel is full" << std::endl;
        }
        size_.fetch_add(1);
    }

    std::optional<Message> receive() override {
        if (is_stopped_) {
            return std::nullopt;
        }

        Message msg;
        if (channel_.pop(msg) == boost::fibers::channel_op_status::success) {
            size_.fetch_sub(1);
            return msg;
        }

        return std::nullopt;
    }


    void handle_messages() override {
        // assert(handler_ != default_handler);
        std::optional<Message> msg_opt;
        while ((msg_opt = receive()) != std::nullopt) {
            auto msg = msg_opt.value();
            handler_(msg);
        }
    }

    void stop() override {
        this->is_stopped_ = true;
    }


private:
    const static uint32_t SIZE = 1 << 17;
    boost::fibers::buffered_channel<Message> channel_;
};

//////////////////////////////////////////// TimerChannel ///////////////////////////////////////////////

namespace asio = boost::asio;

class TimerChannel : public IChannel {
public:
    TimerChannel() : ctx_(1), ctx_guard_(make_work_guard(ctx_)) {
    }

    TimerChannel(const TimerChannel&) = delete;
    TimerChannel& operator=(const TimerChannel&) = delete;

    TimerChannel(TimerChannel&&) = delete;
    TimerChannel& operator=(TimerChannel&&) = delete;

    void set_handler(std::function<void(Message)> handler) override {
        handler_ = handler;
    }

    Sender get_sender() override {
        return Sender(*this);
    }

    uint32_t size() {
        return size_;
    }

    void send(Message msg) override {
        using namespace asio;
        // assert(handler_ != default_handler);
        ++size_;
        if (is_stopped_) {
            --size_;
            return;
        }

        steady_timer* timer = new steady_timer(ctx_);
        timer->expires_after(get_delay());
        timer->async_wait([this, timer, msg = std::move(msg)] (const boost::system::error_code&) {
            delete timer;

            if (this->is_stopped_) {
                --size_;
                return;
            }

            handler_(msg);
            --size_;
        });
    }

    std::optional<Message> receive() override {
        return std::nullopt;
    }

    void handle_messages() override {
        // assert(handler_ != default_handler);
        ctx_.run();
    }

    void stop() override {
        this->is_stopped_ = true;
        ctx_guard_.reset();
        ctx_.stop();
    }

    void close() override {
        if (!is_stopped_) {
            stop();
        }

        while (size_ > 0) {
            ctx_.restart();
            ctx_.run();
        }
    }

private:
    std::chrono::microseconds get_delay() {
        static std::mt19937 gen = std::mt19937(std::random_device()());
        static auto rand_engine = std::uniform_int_distribution<>(
            AVG_DELAY - AVG_DELAY / 2, 
            AVG_DELAY + AVG_DELAY / 2
        );
        return std::chrono::microseconds(rand_engine(gen));
    }

private:
    static const uint32_t AVG_DELAY = 5000;
    asio::io_context ctx_;
    asio::executor_work_guard<asio::io_context::executor_type> ctx_guard_;
};