#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <unordered_set>

struct CellCoord {
    int x, z;
    bool operator==(const CellCoord& o) const { return x == o.x && z == o.z; }
};

struct CellHasher {
    std::size_t operator()(const CellCoord& k) const {
        return std::hash<int>()(k.x) ^ (std::hash<int>()(k.z) << 1);
    }
};

class SpatialGrid {
public:
    static constexpr float CELL_SIZE = 5.0f;

    void UpdateEntity(uint32_t id, float x, float z) {
        int cx = static_cast<int>(x / CELL_SIZE);
        int cz = static_cast<int>(z / CELL_SIZE);

        if (entityCells.find(id) != entityCells.end()) {
            CellCoord old = entityCells[id];
            if (old.x == cx && old.z == cz) return;
            RemoveEntity(id, old);
        }

        CellCoord newC = { cx, cz };
        cells[newC].push_back(id);
        entityCells[id] = newC;
    }

    void RemoveEntity(uint32_t id) {
        if (entityCells.find(id) != entityCells.end()) {
            RemoveEntity(id, entityCells[id]);
            entityCells.erase(id);
        }
    }

    void GetRelevantEntities(float x, float z, std::vector<uint32_t>& outIDs) {
        int cx = static_cast<int>(x / CELL_SIZE);
        int cz = static_cast<int>(z / CELL_SIZE);

        for (int dx = -1; dx <= 1; dx++) {
            for (int dz = -1; dz <= 1; dz++) {
                CellCoord key = { cx + dx, cz + dz };
                if (cells.count(key)) {
                    const auto& list = cells[key];
                    outIDs.insert(outIDs.end(), list.begin(), list.end());
                }
            }
        }
    }

private:
    void RemoveEntity(uint32_t id, CellCoord c) {
        auto& vec = cells[c];
        for (size_t i = 0; i < vec.size(); i++) {
            if (vec[i] == id) {
                vec[i] = vec.back();
                vec.pop_back();
                return;
            }
        }
    }

    std::unordered_map<CellCoord, std::vector<uint32_t>, CellHasher> cells;
    std::unordered_map<uint32_t, CellCoord> entityCells;
};