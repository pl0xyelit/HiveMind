#pragma once

#include <vector>
#include <string>
#include <memory>
#include <random>
#include "Courier.h"
#include "Package.h"

struct Config {
    int rows = 20;
    int cols = 20;
    int maxTicks = 1000;
    int maxStations = 3;
    int clientsCount = 10;
    int drones = 3;
    int robots = 2;
    int scooters = 1;
    int totalPackages = 50;
    int spawnFrequency = 10;
    int displayDelayMs = 100; // milliseconds between ticks when rendering
};

#include "IMapGenerator.h"

class Simulation {
public:
    // Singleton accessors
    static Simulation& getInstance();
    static Simulation* getInstancePtr();
    static bool hasInstance();

    // Construction: keep the existing public constructor to preserve front-facing
    // interfaces (e.g., `Simulation sim("file")`). The constructor will register
    // the created object as the singleton instance; creating a second Simulation
    // will throw.
    explicit Simulation(const std::string& configPath = "simulation_setup.txt");
    ~Simulation();

    // non-copyable / non-movable to enforce single instance semantics
    Simulation(const Simulation&) = delete;
    Simulation& operator=(const Simulation&) = delete;
    Simulation(Simulation&&) = delete;
    Simulation& operator=(Simulation&&) = delete;

    void loadConfig();

    // map generation uses Strategy pattern via IMapGenerator
    void setMapGenerator(std::unique_ptr<IMapGenerator> gen);
    void generateMap();
    int computePriority(Courier* c, Package* p) const;
    int bestPriorityForPackage(Package* p) const;
    void spawnCouriers();
    void run();
    void render(); // display current simulation state to terminal
    bool isAllDelivered();
    void setAllDelivered();
    void loadMapFromFile(std::string mapFile);

#ifdef UNIT_TEST
    // Test-only helpers (exposed only when compiled with -DUNIT_TEST)
public:
    std::vector<std::unique_ptr<Package>>& getPackagesForTest();
    std::vector<Package*>& getPackagePoolForTest();
    std::vector<std::unique_ptr<Courier>>& getCouriersForTest();
    void setCurrentTickForTest(int t);
    void seedRngForTest(unsigned s);
    void callHiveMindDispatchForTest() { hiveMindDispatch(); }
    void callSpawnPackageForTest() { spawnPackage(); }
    void callSpawnCouriersForTest() { spawnCouriers(); }
    // expose private findPath for tests
    std::vector<Vec2> callFindPathForTest(const Vec2 &a, const Vec2 &b, bool canFly) const { return findPath(a,b,canFly); }

    // test-only helpers
    int getDeadAgentsForTest() const { return deadAgents; }
    void callStepForTest() { step(); }
private:
#endif

private:
    Config cfg;
    std::string configPath;

    std::vector<std::string> grid; // rows strings of length cols
    Vec2 basePos;
    std::vector<Vec2> clients;
    std::vector<Vec2> stations;

    std::vector<std::unique_ptr<Courier>> couriers;
    std::vector<std::unique_ptr<Package>> packages; // all packages (spawned)
    std::vector<Package*> packagePool; // pointers to packages currently waiting

    int currentTick = 0;
    int spawnedPackages = 0;

    int operatingCostTotal = 0;
    int deadAgents = 0;

    // active counts of spawned couriers by type (do NOT exceed cfg.* values)
    int activeDrones = 0;
    int activeRobots = 0;
    int activeScooters = 0;

    // spawn policy: spawn a new courier once there are at least this many waiting packages
    // (non-configurable constant for now)
    static constexpr int waitingSpawnThreshold = 4;
    // cooldown (in ticks) between successive automatic spawns triggered by backlog
    static constexpr int spawnCooldownTicks = 5;
    // tick when we last performed an automatic spawn due to backlog (initialized far in the past)
    int lastSpawnTick = -1000000;

    std::mt19937 rng;

    // map generation strategy
    std::unique_ptr<IMapGenerator> mapGenerator;
    bool validateMap() const;

    bool allDelivered = false;
    void spawnPackage();
    void spawnPackagesIfNeeded();

    // Singleton support
    static Simulation* singletonInstance;

    // lazy spawning helpers
    void spawnOneCourier();
    void trySpawnIfNeeded();

    int computeDistance(const Vec2& a, const Vec2& b, bool canFly) const;
    std::vector<Vec2> findPath(const Vec2& a, const Vec2& b, bool canFly) const;
    void hiveMindDispatch();

    void step();
    void writeReport() const;
};