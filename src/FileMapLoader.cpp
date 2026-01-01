#include "FileMapLoader.h"
#include "Simulation.h" // for Config
#include <fstream>
#include <iostream>

FileMapLoader::FileMapLoader(std::string p) : path(std::move(p)) {}

void FileMapLoader::generate(Config& cfg, std::mt19937& /*rng*/, std::vector<std::string>& grid,
                              Vec2& basePos, std::vector<Vec2>& clients, std::vector<Vec2>& stations) {
    std::ifstream in(path);
    if (!in) {
        std::cerr << "Could not open map file: " << path << "\n";
        // produce a minimal empty map so caller can handle it
        grid.assign(cfg.rows, std::string(cfg.cols, '.'));
        basePos = {cfg.cols/2, cfg.rows/2};
        grid[basePos.y][basePos.x] = 'B';
        clients.clear();
        stations.clear();
        return;
    }

    // Read lines and normalize CRLF endings
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }

    if (lines.empty()) {
        std::cerr << "Map file is empty: " << path << "\n";
        grid.assign(cfg.rows, std::string(cfg.cols, '.'));
        basePos = {cfg.cols/2, cfg.rows/2};
        grid[basePos.y][basePos.x] = 'B';
        clients.clear();
        stations.clear();
        return;
    }

    // Make rectangular by padding shorter rows with '.'
    size_t maxlen = 0;
    for (const auto& l : lines) if (l.length() > maxlen) maxlen = l.length();

    grid.clear();
    for (auto& l : lines) {
        std::string row = l;
        if (row.length() < maxlen) row += std::string(maxlen - row.length(), '.');
        grid.push_back(row);
    }

    cfg.rows = (int)grid.size();
    cfg.cols = (int)maxlen;

    // Discover base, clients and stations
    clients.clear();
    stations.clear();
    bool foundBase = false;
    for (int y = 0; y < cfg.rows; ++y) {
        for (int x = 0; x < cfg.cols; ++x) {
            char c = grid[y][x];
            if (c == 'B') {
                basePos = {x, y};
                foundBase = true;
            } else if (c == 'D') {
                clients.push_back({x, y});
            } else if (c == 'S') {
                stations.push_back({x, y});
            }
        }
    }

    if (!foundBase) {
        basePos = {cfg.cols/2, cfg.rows/2};
        grid[basePos.y][basePos.x] = 'B';
        std::cerr << "Map has no base (B); placing base at center (" << basePos.x << "," << basePos.y << ")\n";
    }

    // Keep cfg consistent with map contents (useful for debug runs)
    cfg.clientsCount = (int)clients.size();
    cfg.maxStations = (int)stations.size();

    std::cout << "Loaded map '" << path << "' (" << cfg.rows << "x" << cfg.cols << ") - clients=" << clients.size()
              << " stations=" << stations.size() << "\n";
}
