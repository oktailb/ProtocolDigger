#include <ThreadSafeQueue.h>

void ThreadSafeQueue::push(const PacketData& data) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push_back(data); // Use push_back for queue semantics
    }
    cv_.notify_one();
}

bool ThreadSafeQueue::pop(PacketData& data, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (cv_.wait_for(lock, timeout, [this]{ return !queue_.empty(); })) {
        data = queue_.front();
        queue_.pop_front();
        return true; // Data popped successfully
    } else {
        return false; // Timeout occurred
    }
}

std::deque<PacketData> &ThreadSafeQueue::data()
{
    return queue_;
}
