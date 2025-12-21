#pragma once
#include <unordered_map>
#include <mutex>
#include "Protocol.h"

class EntityManager {
public:
    void UpdateEntity(const Purpose::EntityState &state) {
        std::lock_guard lock(m_mutex);
        m_entities[state.networkID] = state;
    }

    bool GetEntityState(uint32_t id, Purpose::EntityState &outState) {
        std::lock_guard lock(m_mutex);
        auto it = m_entities.find(id);
        if (it != m_entities.end()) {
            outState = it->second;
            return true;
        }
        return false;
    }

    int GetCount() {
        std::lock_guard lock(m_mutex);
        return static_cast<int>(m_entities.size());
    }

private:
    std::unordered_map<uint32_t, Purpose::EntityState> m_entities;
    std::mutex m_mutex;
};
