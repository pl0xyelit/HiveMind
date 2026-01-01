#pragma once

#include "IMapGenerator.h"
#include <string>

class FileMapLoader : public IMapGenerator {
public:
    explicit FileMapLoader(std::string path);
    void generate(Config& cfg, std::mt19937& rng, std::vector<std::string>& grid,
                  Vec2& basePos, std::vector<Vec2>& clients, std::vector<Vec2>& stations) override;
private:
    std::string path;
};
