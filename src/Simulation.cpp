#include "Simulation.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <queue>
#include <cmath>
#include <algorithm>
#include <map>
#include <climits>
#include <thread>
#include <chrono>
#include <iomanip>
#include <stdexcept>
#include <exception>
#include <cstdlib>

#include "Errors.h"
#include "IMapGenerator.h"
#include "FileMapLoader.h"
#include "ProceduralMapGenerator.h"

void Simulation::render()
{
    // clear terminal
    std::cout << "\x1B[2J\x1B[H";

    // copy base grid
    std::vector<std::string> view = grid;

    // overlay couriers
    // std::map<std::pair<int,int>, int> countAt;
    for (auto &c : couriers)
    {
        if (c->isDead())
            continue;
        Vec2 p = c->getPos();
        if (p.x < 0 || p.x >= cfg.rows || p.y < 0 || p.y >= cfg.cols || (p.x == basePos.x && p.y == basePos.y))
            continue;
        // countAt[{p.x,p.y}]++;
        char ch = '?';
        std::string tn = c->typeName();
        if (tn == "Drone")
            ch = '^';
        else if (tn == "Robot")
            ch = 'R';
        else if (tn == "Scooter")
            ch = 's'; // litera mare S din alfabetul latin, altfel aveam conflict cu randarea S-ului
                      // de la statiile de incarcare
        view[p.x][p.y] = ch;
    }

    // show grid
    for (int x = 0; x < cfg.rows; ++x)
    {
        for (int y = 0; y < cfg.cols; ++y)
        {
            char c = view[x][y];
            // color destinations (D), base (B), stations (S) and scooter ('s')
            // ANSI escape codes baby
            if (c == 'D')
                std::cout << "\x1B[1;32mD\x1B[0m"; // green (destination)
            else if (c == 'B')
                std::cout << "\x1B[1;36mB\x1B[0m"; // bright cyan (base)
            else if (c == 'S')
                std::cout << "\x1B[1;33mS\x1B[0m"; // yellow (station)
            else if (c == 's')
                std::cout << "\x1B[1;35mS\x1B[0m"; // magenta (scooter)
            else if (c == '^')
                std::cout << "\x1B[1;34m^\x1B[0m"; // blue (drone)
            else if (c == 'R')
                std::cout << "\x1B[1;92mR\x1B[0m"; // gray (robot)
            else
                std::cout << c;
        }
        std::cout << "\n";
    }

    // stats
    int delivered = 0;
    int delayed = 0;
    for (const auto &p : packages)
    {
        if (p->isDelivered())
        {
            ++delivered;
            if (delivered == cfg.totalPackages)
            {
                setAllDelivered();
            }
            if (p->deliveredAt() > p->getDeadline())
                ++delayed;
        }
    }
    int waiting = (int)packagePool.size();

    int provisionalProfit = 0;
    for (const auto &p : packages)
    {
        if (p->isDelivered())
        {
            provisionalProfit += p->getReward();
            if (p->deliveredAt() > p->getDeadline())
                provisionalProfit -= 50;
        }
    }
    provisionalProfit -= operatingCostTotal;
    provisionalProfit -= deadAgents * 500;

    int activeAgents = 0;
    int carryingAgents = 0;
    for (const auto &c : couriers)
    {
        if (c->isDead())
            continue;
        bool isActive = !c->getPackages().empty() || (c->getPos().x != basePos.x || c->getPos().y != basePos.y);
        if (isActive)
            ++activeAgents;
        if (!c->getPackages().empty())
            ++carryingAgents;
    }

    std::cout << "Tick: " << currentTick << "/" << cfg.maxTicks << "    ";
    std::cout << "Delivered: " << delivered << "    Waiting: " << waiting << "    ";
    std::cout << "Active: " << activeAgents << " (carrying=" << carryingAgents << ")    ";
    std::cout << "Profit (est): " << provisionalProfit << "    ";
    std::cout << "Total agents spawned: " << couriers.size() << "\n";

    std::cout << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(cfg.displayDelayMs));
}
bool Simulation::isAllDelivered()
{
    return allDelivered;
}

void Simulation::setAllDelivered()
{
    allDelivered = true;
}

// Singleton instance pointer (non-owning)
Simulation* Simulation::singletonInstance = nullptr;

Simulation& Simulation::getInstance()
{
    if (!singletonInstance)
        throw std::runtime_error("No Simulation instance available");
    return *singletonInstance;
}

Simulation* Simulation::getInstancePtr()
{
    return singletonInstance;
}

bool Simulation::hasInstance()
{
    return singletonInstance != nullptr;
}

Simulation::Simulation(const std::string &configPath)
    : configPath(configPath)
{
    if (singletonInstance != nullptr)
    {
        throw std::runtime_error("Simulation singleton already exists");
    }
    singletonInstance = this;

    std::random_device rd;
    rng.seed(rd());

    // Defer choosing map generator until after config is loaded.
    mapGenerator = nullptr;
}

Simulation::~Simulation()
{
    if (singletonInstance == this)
        singletonInstance = nullptr;
}

#ifdef UNIT_TEST
std::vector<std::unique_ptr<Package>>& Simulation::getPackagesForTest() { return packages; }
std::vector<Package*>& Simulation::getPackagePoolForTest() { return packagePool; }
std::vector<std::unique_ptr<Courier>>& Simulation::getCouriersForTest() { return couriers; }
void Simulation::setCurrentTickForTest(int t) { currentTick = t; }
void Simulation::seedRngForTest(unsigned s) { rng.seed(s); }
#endif

void Simulation::loadConfig()
{
    std::ifstream in(configPath);
    if (!in)
    {
        throw FileOpenError("Could not open config file: " + configPath + "\n");
    }
    std::string line;
    while (std::getline(in, line))
    {
        std::istringstream iss(line);
        std::string key;
        if (!(iss >> key))
            continue;
        if (key == "MAP_SIZE:")
        {
            iss >> cfg.rows >> cfg.cols;
        }
        else if (key == "MAX_TICKS:")
        {
            iss >> cfg.maxTicks;
        }
        else if (key == "MAX_STATIONS:")
        {
            iss >> cfg.maxStations;
        }
        else if (key == "CLIENTS_COUNT:")
        {
            iss >> cfg.clientsCount;
        }
        else if (key == "DRONES:")
            iss >> cfg.drones;
        else if (key == "ROBOTS:")
            iss >> cfg.robots;
        else if (key == "SCOOTERS:")
            iss >> cfg.scooters;
        else if (key == "TOTAL_PACKAGES:")
            iss >> cfg.totalPackages;
        else if (key == "SPAWN_FREQUENCY:")
            iss >> cfg.spawnFrequency;
        else if (key == "DISPLAY_DELAY_MS:")
            iss >> cfg.displayDelayMs;
        else if (key == "MAP_FILE:")
        {
            std::string mfile;
            iss >> mfile;
            if (!mfile.empty())
            {
                mapGenerator = std::make_unique<FileMapLoader>(mfile);
                std::cout << "Using map file from config: " << mfile << "\n";
            }
        }
    }
    // try
    // {

    // }
    // catch (const FileOpenError &ex)
    // {
    //     std::cerr << "Fatal config parse error: " << ex.what() << std::endl;
    //     std::terminate();
    // }
}

void Simulation::setMapGenerator(std::unique_ptr<IMapGenerator> gen)
{
    mapGenerator = std::move(gen);
}

bool Simulation::validateMap() const
{
    // Ensure base is inside grid
    if (basePos.x < 0 || basePos.x >= cfg.rows || basePos.y < 0 || basePos.y >= cfg.cols)
        return false;
    std::vector<int> visited(cfg.rows * cfg.cols, 0);
    std::queue<Vec2> q;
    q.push(basePos);
    visited[basePos.x * cfg.cols + basePos.y] = 1;
    const int dr[4] = {1, -1, 0, 0};
    const int dc[4] = {0, 0, 1, -1};
    while (!q.empty())
    {
        Vec2 p = q.front();
        q.pop();
        for (int k = 0; k < 4; ++k)
        {
            int nr = p.x + dr[k];
            int nc = p.y + dc[k];
            if (nr < 0 || nc < 0 || nr >= cfg.rows || nc >= cfg.cols)
                continue;
            if (visited[nr * cfg.cols + nc])
                continue;
            if (grid[nr][nc] == '#')
                continue;
            visited[nr * cfg.cols + nc] = 1;
            q.push({nr, nc});
        }
    }
    for (const auto &c : clients)
    {
        if (!visited[c.x * cfg.cols + c.y])
            return false;
    }
    for (const auto &s : stations)
    {
        if (!visited[s.x * cfg.cols + s.y])
            return false;
    }
    return true;
}

void Simulation::generateMap()
{
    if (!mapGenerator)
        mapGenerator = std::make_unique<ProceduralMapGenerator>();

    int attempts = 0;
    const int maxAttempts = 1000;
    bool canRegenerate = (dynamic_cast<ProceduralMapGenerator *>(mapGenerator.get()) != nullptr);

    do
    {
        mapGenerator->generate(cfg, rng, grid, basePos, clients, stations);
        ++attempts;
        if (!validateMap())
        {
            if (!canRegenerate)
            {
                throw MapGenerationError("Loaded map is invalid (not all clients/stations reachable from base).\n");
                break;
            }
            if (attempts >= maxAttempts)
            {
                throw MapGenerationError(
                    std::string("Failed to generate a valid map after ") + std::to_string(attempts) + " attempts; using last map.\n");
            }
        }
    } while (!validateMap() && canRegenerate);
}

void Simulation::loadMapFromFile(std::string mapFile)
{
    std::ifstream in(mapFile);
    if (!in)
    {
        throw FileOpenError("Could not open map file: " + mapFile + "\n");
    }

    // Read lines and normalize CRLF endings
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        lines.push_back(line);
    }

    if (lines.empty())
    {
        throw MapGenerationError("Map file is empty: " + mapFile + "\n");
    }

    // Make rectangular by padding shorter rows with '.'
    size_t maxlen = 0;
    for (const auto &l : lines)
        if (l.length() > maxlen)
            maxlen = l.length();

    grid.clear();
    for (auto &l : lines)
    {
        std::string row = l;
        if (row.length() < maxlen)
            row += std::string(maxlen - row.length(), '.');
        grid.push_back(row);
    }

    cfg.rows = (int)grid.size();
    cfg.cols = (int)maxlen;

    // Discover base, clients and stations
    clients.clear();
    stations.clear();
    bool foundBase = false;
    for (int x = 0; x < cfg.rows; ++x)
    {
        for (int y = 0; y < cfg.cols; ++y)
        {
            char c = grid[x][y];
            if (c == 'B')
            {
                basePos = {x, y};
                foundBase = true;
            }
            else if (c == 'D')
            {
                clients.push_back({x, y});
            }
            else if (c == 'S')
            {
                stations.push_back({x, y});
            }
        }
    }

    if (!foundBase)
    {
        basePos = {cfg.rows / 2, cfg.cols / 2};
        grid[basePos.x][basePos.y] = 'B';
        std::cerr << "Map has no base (B); placing base at center (" << basePos.x << "," << basePos.y << ")\n";
    }

    // Keep cfg consistent with map contents (useful for debug runs)
    cfg.clientsCount = (int)clients.size();
    cfg.maxStations = (int)stations.size();

    std::cout << "Loaded map '" << mapFile << "' (" << cfg.rows << "x" << cfg.cols << ") - clients=" << clients.size()
              << " stations=" << stations.size() << "\n";
    // try
    // {

    // }
    // catch (const FileOpenError &ex) {
    //     std::cerr << "Fatal file parse error: " << ex.what() << std::endl;
    //     std::terminate();
    // }
    // catch (const MapGenerationError &ex) {
    //     std::cerr << "Fatal map generation error: " << ex.what() << std::endl;
    //     std::terminate();
    // }
}

int Simulation::computePriority(Courier *c, Package *p) const
{
    // 1. Basic reachability
    Vec2 courierPos = c->getPos();
    Vec2 dest{p->getDestX(), p->getDestY()};
    int dist = computeDistance(courierPos, dest, c->canFly());
    if (dist < 0)
        return -1e9; // unreachable

    // 2. ETA (ceil division)
    int eta = (dist + c->getSpeed() - 1) / c->getSpeed();
    int deliveryTick = currentTick + eta;

    // 3. Lateness penalty
    int lateness = std::max(0, deliveryTick - p->getDeadline());

    // 4. Operating cost for this delivery
    int opCost = eta * c->getCost();

    // 5. Battery feasibility check
    int returnDist = computeDistance(dest, basePos, c->canFly());
    if (returnDist < 0)
        return -1e9;

    int returnTicks = (returnDist + c->getSpeed() - 1) / c->getSpeed();
    int batteryNeeded = (eta + returnTicks) * c->getConsumption();

    if (c->getBattery() < batteryNeeded)
        return -1e9; // cannot complete safely

    // 6. Final priority score
    int score = p->getReward() - opCost - lateness * 50;

    return score;
}

int Simulation::bestPriorityForPackage(Package *p) const
{
    int best = -1e9;
    for (auto &c : couriers)
    {
        if (c->isDead() || !c->hasFreeCapacity())
            continue;
        int score = computePriority(c.get(), p);
        if (score > best)
            best = score;
    }
    return best;
}

void Simulation::spawnCouriers()
{
    // Start with exactly one Drone at the base (if available in config).
    // Additional couriers will be spawned dynamically as needed during the run.
    couriers.clear();
    activeDrones = activeRobots = activeScooters = 0;

    if (cfg.drones > 0)
    {
        couriers.push_back(std::make_unique<Drone>(basePos));
        ++activeDrones;
        std::cout << "Spawning initial Drone (1/" << cfg.drones << ")\n";
    }
    else if (cfg.robots > 0)
    {
        // If no drones configured, spawn one Robot to get the simulation started.
        couriers.push_back(std::make_unique<Robot>(basePos));
        ++activeRobots;
        std::cout << "Spawning initial Robot (1/" << cfg.robots << ")\n";
    }
    else if (cfg.scooters > 0)
    {
        couriers.push_back(std::make_unique<Scooter>(basePos));
        ++activeScooters;
        std::cout << "Spawning initial Scooter (1/" << cfg.scooters << ")\n";
    }
}

// Spawn one courier of the next available type, respecting per-type limits.
void Simulation::spawnOneCourier()
{
    // If we can spawn more Drones, prefer them first (they were the initial courier).
    if (activeDrones < cfg.drones)
    {
        couriers.push_back(std::make_unique<Drone>(basePos));
        ++activeDrones;
        std::cout << "Spawning Drone (" << activeDrones << "/" << cfg.drones << ")\n";
        return;
    }
    // Then Robots
    if (activeRobots < cfg.robots)
    {
        couriers.push_back(std::make_unique<Robot>(basePos));
        ++activeRobots;
        std::cout << "Spawning Robot (" << activeRobots << "/" << cfg.robots << ")\n";
        return;
    }
    // Then Scooters
    if (activeScooters < cfg.scooters)
    {
        couriers.push_back(std::make_unique<Scooter>(basePos));
        ++activeScooters;
        std::cout << "Spawning Scooter (" << activeScooters << "/" << cfg.scooters << ")\n";
        return;
    }
    // Nothing left to spawn
}

// Try to spawn additional couriers when waiting packages build up.
// New policy: spawn one courier when there are at least `waitingSpawnThreshold` waiting packages.
// To avoid spawning many agents at once, respect a cooldown between automatic spawns.
void Simulation::trySpawnIfNeeded()
{
    int waiting = (int)packagePool.size();
    if (waiting < waitingSpawnThreshold)
        return;

    // enforce cooldown so we don't spawn multiple couriers in quick succession
    if (currentTick - lastSpawnTick < spawnCooldownTicks)
        return;

    // Only spawn if we haven't already spawned all configured couriers
    if (activeDrones + activeRobots + activeScooters < (cfg.drones + cfg.robots + cfg.scooters))
    {
        std::cout << "Waiting packages (" << waiting << ") reached threshold (" << waitingSpawnThreshold << ") - spawning another courier\n";
        spawnOneCourier();
        lastSpawnTick = currentTick;
    }
}

void Simulation::spawnPackage()
{
    if (spawnedPackages >= cfg.totalPackages)
        return;
    std::uniform_int_distribution<int> distClient(0, (int)clients.size() - 1);
    std::uniform_int_distribution<int> reward(200, 800);
    std::uniform_int_distribution<int> deadline(10, 20);
    int idx = distClient(rng);
    Vec2 d = clients[idx];
    int id = spawnedPackages;
    int dl = currentTick + deadline(rng);
    packages.push_back(std::make_unique<Package>(id, d.x, d.y, reward(rng), dl));
    packagePool.push_back(packages.back().get());
    ++spawnedPackages;
}

void Simulation::spawnPackagesIfNeeded()
{
    if (cfg.spawnFrequency <= 0)
        return;
    if ((currentTick % cfg.spawnFrequency) == 0 && spawnedPackages < cfg.totalPackages)
    {
        spawnPackage();
    }
}

int Simulation::computeDistance(const Vec2 &a, const Vec2 &b, bool canFly) const
{
    if (a.x == b.x && a.y == b.y)
        return 0;
    if (canFly)
    {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y);
    }
    // BFS on grid avoiding walls
    std::vector<int> visited(cfg.rows * cfg.cols, 0);
    std::queue<std::pair<Vec2, int>> q;
    q.push({a, 0});
    visited[a.x * cfg.cols + a.y] = 1;
    const int dr[4] = {1, -1, 0, 0};
    const int dc[4] = {0, 0, 1, -1};
    while (!q.empty())
    {
        auto [p, d] = q.front();
        q.pop();
        for (int k = 0; k < 4; ++k)
        {
            int nr = p.x + dr[k];
            int nc = p.y + dc[k];
            if (nr < 0 || nc < 0 || nr >= cfg.rows || nc >= cfg.cols)
                continue;
            if (visited[nr * cfg.cols + nc])
                continue;
            if (grid[nr][nc] == '#')
                continue;
            if (nr == b.x && nc == b.y)
                return d + 1;
            visited[nr * cfg.cols + nc] = 1;
            q.push({{nr, nc}, d + 1});
        }
    }
    return -1; // unreachable
}

std::vector<Vec2> Simulation::findPath(const Vec2 &a, const Vec2 &b, bool canFly) const
{
    std::vector<Vec2> path;
    if (a.x == b.x && a.y == b.y)
        return path;
    if (canFly)
    {
        // direct Manhattan moves
        Vec2 cur = a;
        while (cur.x != b.x || cur.y != b.y)
        {
            if (cur.x < b.x)
                ++cur.x;
            else if (cur.x > b.x)
                --cur.x;
            else if (cur.y < b.y)
                ++cur.y;
            else if (cur.y > b.y)
                --cur.y;
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
    const int dr[4] = {1, -1, 0, 0};
    const int dc[4] = {0, 0, 1, -1};
    bool found = false;
    while (!q.empty() && !found)
    {
        Vec2 p = q.front();
        q.pop();
        for (int k = 0; k < 4; ++k)
        {
            int nr = p.x + dr[k];
            int nc = p.y + dc[k];
            if (nr < 0 || nc < 0 || nr >= cfg.rows || nc >= cfg.cols)
                continue;
            if (visited[nr * cfg.cols + nc])
                continue;
            if (grid[nr][nc] == '#')
                continue;
            visited[nr * cfg.cols + nc] = 1;
            parent[nr * cfg.cols + nc] = p.x * cfg.cols + p.y;
            if (nr == b.x && nc == b.y)
            {
                found = true;
                break;
            }
            q.push({nr, nc});
        }
    }
    if (!found)
        return path;
    // reconstruct
    int idx = b.x * cfg.cols + b.y;
    while (idx != -1 && idx != (a.x * cfg.cols + a.y))
    {
        int row = idx / cfg.cols;
        int col = idx % cfg.cols;
        path.push_back({row, col});
        idx = parent[idx];
    }
    std::reverse(path.begin(), path.end());
    return path;
}
// Hungarian algorithm helper for square cost matrix (minimization)
static std::vector<int> hungarian(const std::vector<std::vector<long long>> &a)
{
    int n = (int)a.size();
    const long long INF = (long long)4e15;
    std::vector<long long> u(n + 1), v(n + 1);
    std::vector<int> p(n + 1), way(n + 1);
    for (int i = 1; i <= n; ++i)
    {
        p[0] = i;
        int j0 = 0;
        std::vector<long long> minv(n + 1, INF);
        std::vector<char> used(n + 1, false);
        do
        {
            used[j0] = true;
            int i0 = p[j0];
            int j1 = 0;
            long long delta = INF;
            for (int j = 1; j <= n; ++j)
            {
                if (used[j])
                    continue;
                long long cur = a[i0 - 1][j - 1] - u[i0] - v[j];
                if (cur < minv[j])
                {
                    minv[j] = cur;
                    way[j] = j0;
                }
                if (minv[j] < delta)
                {
                    delta = minv[j];
                    j1 = j;
                }
            }
            for (int j = 0; j <= n; ++j)
            {
                if (used[j])
                {
                    u[p[j]] += delta;
                    v[j] -= delta;
                }
                else
                {
                    minv[j] -= delta;
                }
            }
            j0 = j1;
        } while (p[j0] != 0);
        do
        {
            int j1 = way[j0];
            p[j0] = p[j1];
            j0 = j1;
        } while (j0);
    }
    std::vector<int> match(n, -1);
    for (int j = 1; j <= n; ++j)
    {
        if (p[j] != 0)
            match[p[j] - 1] = j - 1;
    }
    return match;
}

void Simulation::hiveMindDispatch()
{
    // Build list of waiting packages and available courier slots (one slot per free capacity)
    std::vector<Package *> pkgs = packagePool; // copy pointers
    int P = (int)pkgs.size();
    if (P == 0)
        return;

    std::vector<int> slotToCourier; // map column index -> courier index
    for (size_t i = 0; i < couriers.size(); ++i)
    {
        auto &c = couriers[i];
        if (c->isDead())
            continue;
        int free = c->getCapacity() - (int)c->getPackages().size();
        for (int s = 0; s < free; ++s)
            slotToCourier.push_back((int)i);
    }

    int M = (int)slotToCourier.size();
    if (M == 0)
        return; // no slots available

    // size of square matrix
    int n = std::max(P, M);
    const long long INF_COST = (long long)1e12; // large cost to forbid infeasible assignments

    // build cost matrix: cost = -score for feasible assignments, INF_COST for infeasible
    std::vector<std::vector<long long>> cost(n, std::vector<long long>(n, 0));

    for (int i = 0; i < P; ++i)
    {
        Package *pkg = pkgs[i];
        for (int j = 0; j < M; ++j)
        {
            int courierIdx = slotToCourier[j];
            auto &c = couriers[courierIdx];
            if (c->isDead())
            {
                cost[i][j] = INF_COST;
                continue;
            }
            // quick feasibility checks mirroring previous heuristic
            Vec2 courierPos = c->getPos();
            int dist = computeDistance(courierPos, {pkg->getDestX(), pkg->getDestY()}, c->canFly());
            if (dist < 0) { cost[i][j] = INF_COST; continue; }
            if (c->typeName() == "Drone" && pkg->getReward() < 300) { cost[i][j] = INF_COST; continue; }
            if (c->typeName() == "Robot" && dist > cfg.rows / 3) { cost[i][j] = INF_COST; continue; }

            int ticksNeeded = (dist + c->getSpeed() - 1) / c->getSpeed();
            int batteryNeeded = ticksNeeded * c->getConsumption();
            int minReturnDist = computeDistance({pkg->getDestX(), pkg->getDestY()}, basePos, c->canFly());
            if (minReturnDist < 0) { cost[i][j] = INF_COST; continue; }
            int returnTicks = (minReturnDist + c->getSpeed() - 1) / c->getSpeed();
            int batteryReturn = returnTicks * c->getConsumption();
            if (c->getBattery() < (batteryNeeded + batteryReturn)) { cost[i][j] = INF_COST; continue; }

            int score = computePriority(c.get(), pkg);
            if (score <= -1000000)
                cost[i][j] = INF_COST; // infeasible
            else
                cost[i][j] = - (long long)score; // minimize -score == maximize score
        }
        for (int j = M; j < n; ++j)
        {
            // dummy columns: represent leaving the package unassigned (0 cost)
            cost[i][j] = 0;
        }
    }
    // dummy rows (if any)
    for (int i = P; i < n; ++i)
    {
        for (int j = 0; j < n; ++j)
            cost[i][j] = 0;
    }

    // Solve assignment via Hungarian
    std::vector<int> match = hungarian(cost); // size n, match[row] = col

    // Apply assignments: if row < P and matched col < M and cost not INF_COST, assign
    std::vector<char> assigned(P, false);
    for (int i = 0; i < P; ++i)
    {
        int j = match[i];
        if (j < 0 || j >= M)
            continue;
        if (cost[i][j] >= INF_COST / 2)
            continue; // infeasible
        int courierIdx = slotToCourier[j];
        auto &c = couriers[courierIdx];
        if (c->isDead() || !c->hasFreeCapacity())
            continue; // safety check
        bool ok = c->assignPackage(pkgs[i]);
        if (ok)
            assigned[i] = true;
    }

    // Fallback: if Hungarian assigned nothing and there are waiting packages, pick the best feasible pairs
    int assignedCount = 0;
    for (bool a : assigned)
        if (a)
            ++assignedCount;

    if (assignedCount == 0 && P > 0)
    {
        const long long FALLBACK_THRESHOLD = -1000; // allow small losses to keep system busy
        struct Cand { long long profit; int pi; int col; };
        std::vector<Cand> cands;
        cands.reserve(P * M);
        for (int i = 0; i < P; ++i)
        {
            for (int j = 0; j < M; ++j)
            {
                if (cost[i][j] >= INF_COST / 2) continue;
                long long expectedProfit = -cost[i][j];
                cands.push_back({expectedProfit, i, j});
            }
        }
        std::sort(cands.begin(), cands.end(), [](const Cand &a, const Cand &b){ return a.profit > b.profit; });

        std::vector<char> courierUsed(M, false);
        std::vector<char> pkgUsed(P, false);
        int toTake = std::min(P, M);
        for (const auto &cand : cands)
        {
            if (toTake <= 0) break;
            if (pkgUsed[cand.pi]) continue;
            if (courierUsed[cand.col]) continue;
            if (cand.profit < FALLBACK_THRESHOLD) break; // don't take worse than threshold
            int courierIdx = slotToCourier[cand.col];
            auto &c = couriers[courierIdx];
            if (c->isDead() || !c->hasFreeCapacity()) continue;
            bool ok = c->assignPackage(pkgs[cand.pi]);
            if (ok)
            {
                assigned[cand.pi] = true;
                pkgUsed[cand.pi] = true;
                courierUsed[cand.col] = true;
                --toTake;
            }
        }
    }

    // remove assigned packages from packagePool
    std::vector<Package *> newPool;
    newPool.reserve(packagePool.size());
    for (auto p : packagePool)
    {
        bool wasAssigned = false;
        for (int i = 0; i < P; ++i)
        {
            if (assigned[i] && pkgs[i] == p)
            {
                wasAssigned = true;
                break;
            }
        }
        if (!wasAssigned)
            newPool.push_back(p);
    }
    packagePool.swap(newPool);
    // If we assigned nothing, all packages have been spawned, and there are
    // still waiting packages but no active couriers able to take them,
    // try a last-resort forced assignment before ending the simulation
    // (this relaxes heuristic constraints so packages are attempted one way or another).
    if (assignedCount == 0 && P > 0 && spawnedPackages >= cfg.totalPackages)
    {
        int activeAgents = 0;
        for (const auto &c : couriers)
        {
            if (c->isDead())
                continue;
            bool isActive = !c->getPackages().empty() || (c->getPos().x != basePos.x || c->getPos().y != basePos.y);
            if (isActive)
                ++activeAgents;
        }
        if (activeAgents == 0)
        {
            std::cout << "No active couriers and no feasible assignments detected; attempting forced assignments\n";

            int forcedAssigned = 0;
            // Greedily assign each waiting package to the nearest courier that can reach it (ignoring battery/heuristics)
            for (int i = 0; i < P; ++i)
            {
                if (assigned[i]) continue;
                Package *pkg = pkgs[i];
                int bestCourier = -1;
                int bestDist = INT_MAX;
                for (size_t j = 0; j < couriers.size(); ++j)
                {
                    auto &c = couriers[j];
                    if (c->isDead() || !c->hasFreeCapacity())
                        continue;
                    int dist = computeDistance(c->getPos(), {pkg->getDestX(), pkg->getDestY()}, c->canFly());
                    if (dist < 0) continue; // unreachable
                    if (dist < bestDist)
                    {
                        bestDist = dist;
                        bestCourier = (int)j;
                    }
                }
                if (bestCourier != -1)
                {
                    auto &c = couriers[bestCourier];
                    bool ok = c->assignPackage(pkg);
                    if (ok)
                    {
                        assigned[i] = true;
                        ++forcedAssigned;
                    }
                }
            }

            if (forcedAssigned > 0)
            {
                std::cout << "Forced assignment succeeded: " << forcedAssigned << " packages assigned\n";
                assignedCount += forcedAssigned;
            }
            else
            {
                std::cout << "No available couriers could reach remaining packages; ending simulation early\n";
                setAllDelivered();
            }
        }
    }
}

void Simulation::step()
{
    // spawn packages
    spawnPackagesIfNeeded();

    // maybe spawn additional couriers if backlog grows
    trySpawnIfNeeded();

    // dispatch
    hiveMindDispatch();

    // move couriers and accumulate operating cost per tick
    for (auto &c : couriers)
    {   
        if (c->isDead()) continue;       // don't run movement logic for dead couriers
        operatingCostTotal += c->getCost();
        if (!c->getPackages().empty())
        {
            Package *p = c->getPackages().front();
            Vec2 target{p->getDestX(), p->getDestY()};
            Vec2 cur = c->getPos();
            auto path = findPath(cur, target, c->canFly());
            if (!path.empty())
            {
                int steps = std::min((int)path.size(), c->getSpeed());
                Vec2 next = path[steps - 1];
                c->applyMove(next);
            }
            // check arrival
            if (c->getPos().x == target.x && c->getPos().y == target.y)
            {
                p->markDelivered(currentTick);
                c->removePackage(p);
            }
        }
        else
        {
            // idle at base: if not at base, move back
            if (c->getPos().x != basePos.x || c->getPos().y != basePos.y)
            {
                Vec2 cur = c->getPos();
                auto path = findPath(cur, basePos, c->canFly());
                if (!path.empty())
                {
                    int steps = std::min((int)path.size(), c->getSpeed());
                    Vec2 next = path[steps - 1];
                    c->applyMove(next);
                }
            }
            else
            {
                // recharge at base
                int add = c->getMaxBattery() / 4;
                c->recharge(add);
            }
        }

        // after movement, check if courier is on S or B to recharge a bit
        char cell = grid[c->getPos().x][c->getPos().y];
        if (cell == 'S' || cell == 'B')
        {
            int add = c->getMaxBattery() / 4;
            c->recharge(add);
        }

        // check dead state
        if (c->getBattery() == 0)
        {
            char cellHere = grid[c->getPos().x][c->getPos().y];
            if (cellHere != 'S' && cellHere != 'B')
            {
                c->kill();
                ++deadAgents;
            }
        }
    }

    ++currentTick;
}

void Simulation::run()
{
    try
    {
        loadConfig();
        generateMap();
        // loadMapFromFile("map.txt");
        spawnCouriers();

        // initial render
        render();

        while (currentTick < cfg.maxTicks)
        {
            step();
            render();
            if (Simulation::isAllDelivered())
            {
                break;
            }
        }

        writeReport();
    }
    catch (const FileOpenError &ex)
    {
        std::cerr << "Fatal config parse error: " << ex.what() << std::endl;
        std::terminate();
    }
    catch (const MapGenerationError &ex)
    {
        std::cerr << "Fatal map error: " << ex.what() << std::endl;
        std::terminate();
        // ^^^^ could also be `std::exit(EXIT_FAILURE);`
    }
}

void Simulation::writeReport() const
{
    std::ofstream out("simulation.txt");
    if (!out)
        return;
    int delivered = 0;
    int delayed = 0;
    int lost = 0;
    int profit = 0;
    for (const auto &p : packages)
    {
        if (p->isDelivered())
        {
            ++delivered;
            profit += p->getReward();
            if (p->deliveredAt() > p->getDeadline())
            {
                ++delayed;
                profit -= 50; // penalty
            }
        }
        else
        {
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
