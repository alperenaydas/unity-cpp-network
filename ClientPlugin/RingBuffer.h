#pragma once
#include <mutex>
#include "Protocol.h"

class EntityBuffer {
public:
    static const int BUFFER_SIZE = 1024;

    void Push(const Purpose::EntityState& state) {
        std::lock_guard lock(m_mutex);
        m_buffer[m_head] = state;
        m_head = (m_head + 1) % BUFFER_SIZE;

        if (m_head == m_tail) {
            m_tail = (m_tail + 1) % BUFFER_SIZE;
        }
    }

    bool Pop(Purpose::EntityState& outState) {
        std::lock_guard lock(m_mutex);
        if (m_head == m_tail) return false;

        outState = m_buffer[m_tail];
        m_tail = (m_tail + 1) % BUFFER_SIZE;
        return true;
    }

    void Clear() {
        std::lock_guard lock(m_mutex);
        m_head = 0;
        m_tail = 0;
    }

private:
    Purpose::EntityState m_buffer[BUFFER_SIZE];
    int m_head = 0;
    int m_tail = 0;
    std::mutex m_mutex;
};