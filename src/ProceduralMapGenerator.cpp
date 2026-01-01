#include "ProceduralMapGenerator.h"
#include "Simulation.h" // for Config
#include <random>

ProceduralMapGenerator::ProceduralMapGenerator(double wallProbability)
    : wallProb(wallProbability) {}

void ProceduralMapGenerator::generate(Config& cfg, std::mt19937& rng, std::vector<std::string>& grid,
                                      Vec2& basePos, std::vector<Vec2>& clients, std::vector<Vec2>& stations) {
    grid.assign(cfg.rows, std::string(cfg.cols, '.'));
    // Place base in center (x=col, y=row)
    basePos = {cfg.cols/2, cfg.rows/2};
    grid[basePos.y][basePos.x] = 'B';

    // random placements
    std::uniform_int_distribution<int> rx(0, cfg.cols-1);
    std::uniform_int_distribution<int> ry(0, cfg.rows-1);

    // place clients
    clients.clear();
    for (int i = 0; i < cfg.clientsCount; ++i) {
        while (true) {
            int x = rx(rng);
            int y = ry(rng);
            if (grid[y][x] == '.') {
                grid[y][x] = 'D';
                clients.push_back({x,y});
                break;
            }
        }
    }

    // place stations
    stations.clear();
    for (int i = 0; i < cfg.maxStations; ++i) {
        while (true) {
            int x = rx(rng);
            int y = ry(rng);
            if (grid[y][x] == '.') {
                grid[y][x] = 'S';
                stations.push_back({x,y});
                break;
            }
        }
    }

    // add some walls
    std::uniform_real_distribution<> frac(0.0, 1.0);
    for (int y = 0; y < cfg.rows; ++y) {
        for (int x = 0; x < cfg.cols; ++x) {
            if (grid[y][x] == '.' && frac(rng) < wallProb) grid[y][x] = '#';
        }
    }
}
