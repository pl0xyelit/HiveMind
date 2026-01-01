#pragma once

#include "IMapGenerator.h"

class ProceduralMapGenerator : public IMapGenerator {
public:
    explicit ProceduralMapGenerator(double wallProbability = 0.08);
    void generate(Config& cfg, std::mt19937& rng, std::vector<std::string>& grid,
                  Vec2& basePos, std::vector<Vec2>& clients, std::vector<Vec2>& stations) override;
private:
    double wallProb;
};
