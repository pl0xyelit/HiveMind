#pragma once

#include <vector>
#include <string>
#include <random>
#include "Courier.h" // for Vec2

struct Config; // forward declaration; defined in Simulation.h

class IMapGenerator {
public:
    virtual ~IMapGenerator() = default;

    // Generate fills grid, basePos, clients and stations. It may also update cfg (e.g., rows/cols)
    virtual void generate(Config& cfg,
                          std::mt19937& rng,
                          std::vector<std::string>& grid,
                          Vec2& basePos,
                          std::vector<Vec2>& clients,
                          std::vector<Vec2>& stations) = 0;
};
