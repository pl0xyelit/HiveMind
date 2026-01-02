#include "Simulation.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <queue>
#include <cmath>
#include <algorithm>
#include <map>
#include <thread>
#include <chrono>
#include <iomanip>

#include "IMapGenerator.h"
#include "FileMapLoader.h"
#include "ProceduralMapGenerator.h"

void Simulation::render() {
    // clear terminal
    std::cout << "\x1B[2J\x1B[H";

    // copy base grid
    std::vector<std::string> view = grid;

    // overlay couriers
    //std::map<std::pair<int,int>, int> countAt;
    for (auto& c : couriers) {
        if (c->isDead()) continue;
        Vec2 p = c->getPos();
        if (p.x < 0 || p.x >= cfg.rows || p.y < 0 || p.y >= cfg.cols || (p.x == basePos.x && p.y == basePos.y)) continue;
        //countAt[{p.x,p.y}]++;
        char ch = '?';
        std::string tn = c->typeName();
        if (tn == "Drone") ch = '^';
        else if (tn == "Robot") ch = 'R';
        else if (tn == "Scooter") ch = 'S'; // litera mare S din alfabetul latin, altfel aveam conflict cu randarea S-ului
                                            // de la statiile de incarcare
        view[p.x][p.y] = ch; 
    }

    // show grid
    for (int x = 0; x < cfg.rows; ++x) {
        for (int y = 0; y < cfg.cols; ++y) {
            char c = view[x][y];
            // color destinations (D), base (B), stations (S) and scooter ('s')
            // ANSI escape codes baby
            if (c == 'D') std::cout << "\x1B[1;32mD\x1B[0m";      // green (destination)
            else if (c == 'B') std::cout << "\x1B[1;36mB\x1B[0m"; // bright cyan (base)
            else if (c == 'S') std::cout << "\x1B[1;33mS\x1B[0m"; // yellow (station)
            else if (c == 'S') std::cout << "\x1B[1;35mS\x1B[0m"; // magenta (scooter)
            else if (c == '^') std::cout << "\x1B[1;34m^\x1B[0m"; // blue (drone)
            else if (c == 'R') std::cout << "\x1B[1;92mR\x1B[0m"; // gray (robot)
            else std::cout << c;
        }
        std::cout << "\n";
    }

    // stats
    int delivered = 0;
    int delayed = 0;
    for (const auto& p : packages) {
        if (p->isDelivered()) {
            ++delivered;
            if(delivered == cfg.totalPackages) {
                setAllDelivered();
            }
            if (p->deliveredAt() > p->getDeadline()) ++delayed;
        }
    }
    int waiting = (int)packagePool.size();

    int provisionalProfit = 0;
    for (const auto& p : packages) {
        if (p->isDelivered()) {
            provisionalProfit += p->getReward();
            if (p->deliveredAt() > p->getDeadline()) provisionalProfit -= 50;
        }
    }
    provisionalProfit -= operatingCostTotal;
    provisionalProfit -= deadAgents * 500;

    std::cout << "Tick: " << currentTick << "/" << cfg.maxTicks << "    ";
    std::cout << "Delivered: " << delivered << "    Waiting: " << waiting << "    ";
    std::cout << "Profit (est): " << provisionalProfit << "\n";

    std::cout << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(cfg.displayDelayMs));
}
bool Simulation::isAllDelivered() {
    return allDelivered;
}

void Simulation::setAllDelivered() {
    allDelivered = true;
}

Simulation::Simulation(const std::string& configPath)
    : configPath(configPath)
{
    std::random_device rd;
    rng.seed(rd());

    // Defer choosing map generator until after config is loaded.
    mapGenerator = nullptr;
}

bool Simulation::loadConfig() {
    std::ifstream in(configPath);
    if (!in) {
        std::cerr << "Could not open config file: " << configPath << "\n";
        return false;
    }
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream iss(line);
        std::string key;
        if (!(iss >> key)) continue;
        if (key == "MAP_SIZE:") {
            iss >> cfg.rows >> cfg.cols;
        } else if (key == "MAX_TICKS:") {
            iss >> cfg.maxTicks;
        } else if (key == "MAX_STATIONS:") {
            iss >> cfg.maxStations;
        } else if (key == "CLIENTS_COUNT:") {
            iss >> cfg.clientsCount;
        } else if (key == "DRONES:") iss >> cfg.drones;
        else if (key == "ROBOTS:") iss >> cfg.robots;
        else if (key == "SCOOTERS:") iss >> cfg.scooters;
        else if (key == "TOTAL_PACKAGES:") iss >> cfg.totalPackages;
        else if (key == "SPAWN_FREQUENCY:") iss >> cfg.spawnFrequency;
        else if (key == "DISPLAY_DELAY_MS:") iss >> cfg.displayDelayMs;
        else if (key == "MAP_FILE:") {
            std::string mfile; iss >> mfile;
            if (!mfile.empty()) {
                mapGenerator = std::make_unique<FileMapLoader>(mfile);
                std::cout << "Using map file from config: " << mfile << "\n";
            }
        }
    }
    return true;
} 

void Simulation::setMapGenerator(std::unique_ptr<IMapGenerator> gen) {
    mapGenerator = std::move(gen);
}

bool Simulation::validateMap() const {
    // Ensure base is inside grid
    if (basePos.x < 0 || basePos.x >= cfg.rows || basePos.y < 0 || basePos.y >= cfg.cols) return false;
    std::vector<int> visited(cfg.rows * cfg.cols, 0);
    std::queue<Vec2> q;
    q.push(basePos);
    visited[basePos.x * cfg.cols + basePos.y] = 1;
    const int dr[4] = {1,-1,0,0};
    const int dc[4] = {0,0,1,-1};
    while (!q.empty()) {
        Vec2 p = q.front(); q.pop();
        for (int k = 0; k < 4; ++k) {
            int nr = p.x + dr[k];
            int nc = p.y + dc[k];
            if (nr < 0 || nc < 0 || nr >= cfg.rows || nc >= cfg.cols) continue;
            if (visited[nr * cfg.cols + nc]) continue;
            if (grid[nr][nc] == '#') continue;
            visited[nr * cfg.cols + nc] = 1;
            q.push({nr, nc});
        }
    }
    for (const auto& c : clients) {
        if (!visited[c.x * cfg.cols + c.y]) return false;
    }
    for (const auto& s : stations) {
        if (!visited[s.x * cfg.cols + s.y]) return false;
    }
    return true;
}

void Simulation::generateMap() {
    if (!mapGenerator) mapGenerator = std::make_unique<ProceduralMapGenerator>();

    int attempts = 0;
    const int maxAttempts = 1000;
    bool canRegenerate = (dynamic_cast<ProceduralMapGenerator*>(mapGenerator.get()) != nullptr);

    do {
        mapGenerator->generate(cfg, rng, grid, basePos, clients, stations);
        ++attempts;
        if (!validateMap()) {
            if (!canRegenerate) {
                std::cerr << "Loaded map is invalid (not all clients/stations reachable from base).\n";
                break;
            }
            if (attempts >= maxAttempts) {
                std::cerr << "Failed to generate a valid map after " << attempts << " attempts; using last map.\n";
                break;
            }
        }
    } while (!validateMap() && canRegenerate);
}

void Simulation::loadMapFromFile(std::string mapFile) {
    std::ifstream in(mapFile);
    if (!in) {
        std::cerr << "Could not open map file: " << mapFile << "\n";
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
        std::cerr << "Map file is empty: " << mapFile << "\n";
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
    for (int x = 0; x < cfg.rows; ++x) {
        for (int y = 0; y < cfg.cols; ++y) {
            char c = grid[x][y];
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
        basePos = {cfg.rows/2, cfg.cols/2};
        grid[basePos.x][basePos.y] = 'B';
        std::cerr << "Map has no base (B); placing base at center (" << basePos.x << "," << basePos.y << ")\n";
    }

    // Keep cfg consistent with map contents (useful for debug runs)
    cfg.clientsCount = (int)clients.size();
    cfg.maxStations = (int)stations.size();

    std::cout << "Loaded map '" << mapFile << "' (" << cfg.rows << "x" << cfg.cols << ") - clients=" << clients.size()
              << " stations=" << stations.size() << "\n";
}

void Simulation::spawnCouriers() {
    couriers.clear();
    for (int i = 0; i < cfg.drones; ++i) {
        couriers.push_back(std::make_unique<Drone>(basePos));
    }
    for (int i = 0; i < cfg.robots; ++i) {
        couriers.push_back(std::make_unique<Robot>(basePos));
    }
    for (int i = 0; i < cfg.scooters; ++i) {
        couriers.push_back(std::make_unique<Scooter>(basePos));
    }
}

void Simulation::spawnPackage() {
    if (spawnedPackages >= cfg.totalPackages) return;
    std::uniform_int_distribution<int> distClient(0, (int)clients.size()-1);
    std::uniform_int_distribution<int> reward(200, 800);
    std::uniform_int_distribution<int> deadline(10, 20);
    int idx = distClient(rng);
    Vec2 d = clients[idx];
    int id = spawnedPackages;
    packages.push_back(std::make_unique<Package>(id, d.x, d.y, reward(rng), deadline(rng)));
    packagePool.push_back(packages.back().get());
    ++spawnedPackages;
}

void Simulation::spawnPackagesIfNeeded() {
    if (cfg.spawnFrequency <= 0) return;
    if ((currentTick % cfg.spawnFrequency) == 0 && spawnedPackages < cfg.totalPackages) {
        spawnPackage();
    }
}

int Simulation::computeDistance(const Vec2& a, const Vec2& b, bool canFly) const {
    if (a.x == b.x && a.y == b.y) return 0;
    if (canFly) {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y);
    }
    // BFS on grid avoiding walls
    std::vector<int> visited(cfg.rows * cfg.cols, 0);
    std::queue<std::pair<Vec2,int>> q;
    q.push({a, 0});
    visited[a.x * cfg.cols + a.y] = 1;
    const int dr[4] = {1,-1,0,0};
    const int dc[4] = {0,0,1,-1};
    while (!q.empty()) {
        auto [p, d] = q.front(); q.pop();
        for (int k = 0; k < 4; ++k) {
            int nr = p.x + dr[k];
            int nc = p.y + dc[k];
            if (nr < 0 || nc < 0 || nr >= cfg.rows || nc >= cfg.cols) continue;
            if (visited[nr * cfg.cols + nc]) continue;
            if (grid[nr][nc] == '#') continue;
            if (nr == b.x && nc == b.y) return d+1;
            visited[nr * cfg.cols + nc] = 1;
            q.push({{nr, nc}, d+1});
        }
    }
    return -1; // unreachable
}

std::vector<Vec2> Simulation::findPath(const Vec2& a, const Vec2& b, bool canFly) const {
    std::vector<Vec2> path;
    if (a.x == b.x && a.y == b.y) return path;
    if (canFly) {
        // direct Manhattan moves
        Vec2 cur = a;
        while (cur.x != b.x || cur.y != b.y) {
            if (cur.x < b.x) ++cur.x;
            else if (cur.x > b.x) --cur.x;
            else if (cur.y < b.y) ++cur.y;
            else if (cur.y > b.y) --cur.y;
            path.push_back(cur);
        }
        return path;
    }
    // BFS with parent pointers
    std::vector<int> visited(cfg.rows * cfg.cols, 0);
    std::vector<int> parent(cfg.rows * cfg.cols, -1);
    std::queue<Vec2> q;
    q.push(a);
    visited[a.x * cfg.cols + a.y] = 1;
    const int dr[4] = {1,-1,0,0};
    const int dc[4] = {0,0,1,-1};
    bool found = false;
    while (!q.empty() && !found) {
        Vec2 p = q.front(); q.pop();
        for (int k = 0; k < 4; ++k) {
            int nr = p.x + dr[k];
            int nc = p.y + dc[k];
            if (nr < 0 || nc < 0 || nr >= cfg.rows || nc >= cfg.cols) continue;
            if (visited[nr * cfg.cols + nc]) continue;
            if (grid[nr][nc] == '#') continue;
            visited[nr * cfg.cols + nc] = 1;
            parent[nr * cfg.cols + nc] = p.x * cfg.cols + p.y;
            if (nr == b.x && nc == b.y) { found = true; break; }
            q.push({nr, nc});
        }
    }
    if (!found) return path;
    // reconstruct
    int idx = b.x * cfg.cols + b.y;
    while (idx != -1 && idx != (a.x * cfg.cols + a.y)) {
        int row = idx / cfg.cols;
        int col = idx % cfg.cols;
        path.push_back({row, col});
        idx = parent[idx];
    }
    std::reverse(path.begin(), path.end());
    return path;
}
void Simulation::hiveMindDispatch() {
    // Very simple heuristic: for each waiting package, assign to the courier with free capacity
    // that can reach the destination and still have battery to reach a station/base.
    for (auto it = packagePool.begin(); it != packagePool.end();) {
        Package* pkg = *it;
        bool assigned = false;
        int bestIdx = -1;
        int bestScore = -1000000000;
        for (size_t i = 0; i < couriers.size(); ++i) {
            auto& c = couriers[i];
            if (!c->hasFreeCapacity() || c->isDead()) continue;
            Vec2 courierPos = c->getPos();
            int dist = computeDistance(courierPos, {pkg->getDestX(), pkg->getDestY()}, c->canFly());
            if (dist < 0) continue;
            int ticksNeeded = (dist + c->getSpeed() - 1) / c->getSpeed();
            int batteryNeeded = ticksNeeded * c->getConsumption();
            int minReturnDist = computeDistance({pkg->getDestX(), pkg->getDestY()}, basePos, c->canFly());
            if (minReturnDist < 0) continue;
            int returnTicks = (minReturnDist + c->getSpeed() - 1) / c->getSpeed();
            int batteryReturn = returnTicks * c->getConsumption();
            if (c->getBattery() < (batteryNeeded + batteryReturn)) continue;

            // estimate score: reward minus operating cost minus lateness penalty
            int estDeliveryTick = currentTick + ticksNeeded;
            int lateness = std::max(0, estDeliveryTick - pkg->getDeadline());
            int estCost = ticksNeeded * c->getCost();
            int score = pkg->getReward() - estCost - (lateness * 50);

            if (score > bestScore) {
                bestScore = score;
                bestIdx = (int)i;
            }
            
        }
        if (bestIdx >= 0 && bestScore > -100000) {
            bool ok = couriers[bestIdx]->assignPackage(pkg);
            if (ok) {
                it = packagePool.erase(it);
                assigned = true;
            }
        }
        if (!assigned) ++it;
    }
}

void Simulation::step() {
    // spawn packages
    spawnPackagesIfNeeded();

    // dispatch
    hiveMindDispatch();

    // move couriers and accumulate operating cost per tick
    for (auto& c : couriers) {
        if (!c->isDead()) operatingCostTotal += c->getCost();
        if (!c->getPackages().empty()) {
            Package* p = c->getPackages().front();
            Vec2 target{p->getDestX(), p->getDestY()};
            Vec2 cur = c->getPos();
            auto path = findPath(cur, target, c->canFly());
            if (!path.empty()) {
                int steps = std::min((int)path.size(), c->getSpeed());
                Vec2 next = path[steps-1];
                c->applyMove(next);
            }
            // check arrival
            if (c->getPos().x == target.x && c->getPos().y == target.y) {
                p->markDelivered(currentTick);
                c->removePackage(p);
            }
        } else {
            // idle at base: if not at base, move back
            if (c->getPos().x != basePos.x || c->getPos().y != basePos.y) {
                Vec2 cur = c->getPos();
                auto path = findPath(cur, basePos, c->canFly());
                if (!path.empty()) {
                    int steps = std::min((int)path.size(), c->getSpeed());
                    Vec2 next = path[steps-1];
                    c->applyMove(next);
                }
            } else {
                // recharge at base
                int add = c->getMaxBattery() / 4;
                c->recharge(add);
            }
        }

        // after movement, check if courier is on S or B to recharge a bit
        char cell = grid[c->getPos().x][c->getPos().y];
        if (cell == 'S' || cell == 'B') {
            int add = c->getMaxBattery() / 4;
            c->recharge(add);
        }

        // check dead state
        if (c->getBattery() == 0) {
            char cellHere = grid[c->getPos().x][c->getPos().y];
            if (cellHere != 'S' && cellHere != 'B') {
                c->kill();
                ++deadAgents;
            }
        }
    }

    ++currentTick;
}

void Simulation::run() {
    if (!loadConfig()) return;
    generateMap();
    //loadMapFromFile("map.txt");
    spawnCouriers();

    // initial render
    render();

    while (currentTick < cfg.maxTicks) {
        step();
        render();
        if(Simulation::isAllDelivered()) {
            break;
        }
    }

    writeReport();
}

void Simulation::writeReport() const {
    std::ofstream out("simulation.txt");
    if (!out) return;
    int delivered = 0;
    int delayed = 0;
    int lost = 0;
    int profit = 0;
    for (const auto& p : packages) {
        if (p->isDelivered()) {
            ++delivered;
            profit += p->getReward();
            if (p->deliveredAt() > p->getDeadline()) {
                ++delayed;
                profit -= 50; // penalty
            }
        } else {
            ++lost;
            profit -= 200; // undelivered penalty
        }
    }
    // subtract operating cost
    profit -= operatingCostTotal;
    // dead agent penalty
    profit -= deadAgents * 500;

    out << "Delivered: " << delivered << "\n";
    out << "Delayed: " << delayed << "\n";
    out << "Lost: " << lost << "\n";
    out << "Operating cost: " << operatingCostTotal << "\n";
    out << "Dead agents: " << deadAgents << "\n";
    out << "Profit: " << profit << "\n";
}
