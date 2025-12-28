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

void Simulation::render() {
    // clear terminal
    std::cout << "\x1B[2J\x1B[H";

    // copy base grid
    std::vector<std::string> view = grid;

    // overlay couriers
    std::map<std::pair<int,int>, int> countAt;
    for (auto& c : couriers) {
        if (c->isDead()) continue;
        Vec2 p = c->getPos();
        if (p.x < 0 || p.x >= cfg.cols || p.y < 0 || p.y >= cfg.rows) continue;
        countAt[{p.x,p.y}]++;
        char ch = '?';
        std::string tn = c->typeName();
        if (tn == "Drone") ch = '^';
        else if (tn == "Robot") ch = 'R';
        else if (tn == "Scooter") ch = 'S';
        view[p.y][p.x] = ch;
    }

    // show grid
    for (int y = 0; y < cfg.rows; ++y) {
        std::cout << view[y] << "\n";
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
    }
    return true;
} 

void Simulation::generateMap() {
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
            if (grid[y][x] == '.' && frac(rng) < 0.08) grid[y][x] = '#';
        }
    }
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
    visited[a.y * cfg.cols + a.x] = 1;
    const int dx[4] = {1,-1,0,0};
    const int dy[4] = {0,0,1,-1};
    while (!q.empty()) {
        auto [p, d] = q.front(); q.pop();
        for (int k = 0; k < 4; ++k) {
            int nx = p.x + dx[k];
            int ny = p.y + dy[k];
            if (nx < 0 || ny < 0 || nx >= cfg.cols || ny >= cfg.rows) continue;
            if (visited[ny * cfg.cols + nx]) continue;
            if (grid[ny][nx] == '#') continue;
            if (nx == b.x && ny == b.y) return d+1;
            visited[ny * cfg.cols + nx] = 1;
            q.push({{nx, ny}, d+1});
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
    visited[a.y * cfg.cols + a.x] = 1;
    const int dx[4] = {1,-1,0,0};
    const int dy[4] = {0,0,1,-1};
    bool found = false;
    while (!q.empty() && !found) {
        Vec2 p = q.front(); q.pop();
        for (int k = 0; k < 4; ++k) {
            int nx = p.x + dx[k];
            int ny = p.y + dy[k];
            if (nx < 0 || ny < 0 || nx >= cfg.cols || ny >= cfg.rows) continue;
            if (visited[ny * cfg.cols + nx]) continue;
            if (grid[ny][nx] == '#') continue;
            visited[ny * cfg.cols + nx] = 1;
            parent[ny * cfg.cols + nx] = p.y * cfg.cols + p.x;
            if (nx == b.x && ny == b.y) { found = true; break; }
            q.push({nx, ny});
        }
    }
    if (!found) return path;
    // reconstruct
    int idx = b.y * cfg.cols + b.x;
    while (idx != -1 && idx != (a.y * cfg.cols + a.x)) {
        int px = idx % cfg.cols;
        int py = idx / cfg.cols;
        path.push_back({px, py});
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
        char cell = grid[c->getPos().y][c->getPos().x];
        if (cell == 'S' || cell == 'B') {
            int add = c->getMaxBattery() / 4;
            c->recharge(add);
        }

        // check dead state
        if (c->getBattery() == 0) {
            char cellHere = grid[c->getPos().y][c->getPos().x];
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
