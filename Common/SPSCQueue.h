#pragma once
#include <vector>
#include <atomic>

template <typename T>
class SPSCQueue {
public:
    explicit SPSCQueue(size_t size = 1024)
        : buffer(size), capacity(size), head(0), tail(0) {}

    bool Push(const T& item) {
        size_t currentHead = head.load(std::memory_order_relaxed);
        size_t nextHead = (currentHead + 1) % capacity;

        if (nextHead == tail.load(std::memory_order_acquire)) {
            return false; 
        }

        buffer[currentHead] = item;

        head.store(nextHead, std::memory_order_release); 
        return true;
    }

    bool Pop(T& outItem) {
        size_t currentTail = tail.load(std::memory_order_relaxed);

        if (currentTail == head.load(std::memory_order_acquire)) {
            return false; 
        }

        outItem = buffer[currentTail];

        tail.store((currentTail + 1) % capacity, std::memory_order_release);
        return true;
    }

private:
    std::vector<T> buffer;
    size_t capacity;
    std::atomic<size_t> head;
    std::atomic<size_t> tail;
};