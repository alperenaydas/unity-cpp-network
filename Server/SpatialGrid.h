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

struct EntityLocation {
    CellCoord cell;
    size_t vectorIndex;
};

class SpatialGrid {
public:
    static constexpr float CELL_SIZE = 25.0f;

    void UpdateEntity(uint32_t id, float x, float z) {
        int cx = static_cast<int>(x / CELL_SIZE);
        int cz = static_cast<int>(z / CELL_SIZE);
        CellCoord newCell = { cx, cz };

        auto it = entityLookup.find(id);
        if (it != entityLookup.end()) {
            if (it->second.cell == newCell) return;
            RemoveEntity(id);
        }

        std::vector<uint32_t>& vec = cells[newCell];
        vec.push_back(id);

        entityLookup[id] = { newCell, vec.size() - 1 };
    }

    void RemoveEntity(uint32_t id) {
        auto it = entityLookup.find(id);
        if (it == entityLookup.end()) return;

        EntityLocation loc = it->second;
        std::vector<uint32_t>& vec = cells[loc.cell];

        uint32_t lastID = vec.back();
        size_t indexToRemove = loc.vectorIndex;

        if (id != lastID) {
            vec[indexToRemove] = lastID;

            entityLookup[lastID].vectorIndex = indexToRemove;
        }

        vec.pop_back();

        if (vec.empty()) {
            cells.erase(loc.cell);
        }

        entityLookup.erase(id);
    }

    void GetRelevantEntities(float x, float z, std::vector<uint32_t>& outIDs) {
        int cx = static_cast<int>(x / CELL_SIZE);
        int cz = static_cast<int>(z / CELL_SIZE);

        for (int dx = -1; dx <= 1; dx++) {
            for (int dz = -1; dz <= 1; dz++) {
                CellCoord key = { cx + dx, cz + dz };

                auto it = cells.find(key);
                if (it != cells.end()) {
                    const auto& list = it->second;
                    outIDs.insert(outIDs.end(), list.begin(), list.end());
                }
            }
        }
    }

private:
    std::unordered_map<CellCoord, std::vector<uint32_t>, CellHasher> cells;
    std::unordered_map<uint32_t, EntityLocation> entityLookup;
};