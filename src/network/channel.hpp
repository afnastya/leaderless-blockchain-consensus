#pragma once

#include <glog/logging.h>
#include <boost/fiber/buffered_channel.hpp>
#include <optional>

#include "../core/message.hpp"

template <typename T>
class Sender;

template <typename T>
class Receiver;

template <typename T>
class Channel {
    friend class Sender<T>;
    friend class Receiver<T>;

public:
    // size should be the power of 2
    Channel(uint32_t size = SIZE) : channel_(size) {
    }

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    Channel(Channel&&) = default;
    Channel& operator=(Channel&&) = default;

    Sender<T> get_sender() {
        return Sender<T>(*this);
    }

    Receiver<T> get_receiver() {
        return Receiver<T>(*this);
    }

    void close() {
        channel_.close();
    }

private:
    const static uint32_t SIZE = 1 << 17;
    boost::fibers::buffered_channel<T> channel_;
    std::atomic<uint32_t> size_{0};
};

template <typename T>
class Sender {
public:
    Sender(Channel<T>& channel) : channel_(channel) {
    }

    void send(T object) {
        if (channel_.channel_.try_push(std::move(object)) == boost::fibers::channel_op_status::full) {
            LOG(FATAL) << "channel is full" << std::endl;
        }
        channel_.size_.fetch_add(1);
    }

private:
    Channel<T>& channel_;
};

template <typename T>
class Receiver {
public:
    Receiver(Channel<T>& channel) : channel_(channel) {
    }

    std::optional<T> receive() {
        T object;
        if (channel_.channel_.pop(object) == boost::fibers::channel_op_status::success) {
            channel_.size_.fetch_sub(1);
            return object;
        }

        return std::nullopt;
    }

    uint32_t size() {
        return channel_.size_;
    }

    void close() {
        channel_.close();
    }

private:
    Channel<T>& channel_;
};