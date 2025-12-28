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

class Simulation {
public:
    explicit Simulation(const std::string& configPath = "simulation_setup.txt");
    bool loadConfig();
    void generateMap();
    void spawnCouriers();
    void run();
    void render(); // display current simulation state to terminal
    bool isAllDelivered();
    void setAllDelivered();

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

    std::mt19937 rng;
    bool allDelivered = false;
    void spawnPackage();
    void spawnPackagesIfNeeded();

    int computeDistance(const Vec2& a, const Vec2& b, bool canFly) const;
    std::vector<Vec2> findPath(const Vec2& a, const Vec2& b, bool canFly) const;
    void hiveMindDispatch();

    void step();
    void writeReport() const;
};