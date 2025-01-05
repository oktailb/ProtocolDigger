#pragma once
#include <stdint.h>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <deque>

struct PacketData {
    uint32_t len;
    struct timeval ts;
    std::vector<uint8_t> payload;
};

// thread-safe queue
class ThreadSafeQueue {
public:
    void push(const PacketData& data);
    bool pop(PacketData& data, std::chrono::milliseconds timeout);
    std::deque<PacketData> &data();

private:
    std::deque<PacketData> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
};
