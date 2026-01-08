#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>

#include "../src/Simulation.h"

#define ASSERT(cond) do { if (!(cond)) { std::cerr << "ASSERT FAILED: " << #cond << " (" << __FILE__ << ":" << __LINE__ << ")\n"; return false; } } while(0)

static std::string makeTempPath(const std::string &base) {
    return "/tmp/" + base + "_" + std::to_string(getpid()) + ".txt";
}

static void writeFile(const std::string &path, const std::string &content) {
    std::ofstream out(path);
    out << content;
    out.close();
}

bool test_findPath_flying_and_blocked() {
    std::string cfg = makeTempPath("cfg_findpath");
    writeFile(cfg,
        "MAP_SIZE: 5 5\n"
        "MAX_TICKS: 10\n"
        "DRONES: 0\n"
        "ROBOTS: 0\n"
        "SCOOTERS: 0\n"
        "TOTAL_PACKAGES: 0\n"
        "SPAWN_FREQUENCY: 0\n"
    );
    std::string map = makeTempPath("map_findpath");
    writeFile(map,
        "B....\n"
        ".....\n"
        "..D..\n"
        ".....\n"
        ".....\n"
    );

    Simulation sim(cfg);
    sim.loadConfig();
    sim.loadMapFromFile(map);

    auto p = sim.callFindPathForTest({0,0}, {2,2}, true);
    ASSERT(!p.empty());
    ASSERT(p.back().x == 2 && p.back().y == 2);

    // blocked map
    writeFile(map,
        "B#...\n"
        "###..\n"
        "..D..\n"
        ".....\n"
        ".....\n"
    );
    sim.loadMapFromFile(map);
    auto p2 = sim.callFindPathForTest({0,0}, {2,2}, false);
    ASSERT(p2.empty());
    return true;
}

bool test_spawnPackage_deadline_relative() {
    std::string cfg = makeTempPath("cfg_spawn");
    writeFile(cfg,
        "MAP_SIZE: 5 5\n"
        "MAX_TICKS: 100\n"
        "DRONES: 1\n"
        "ROBOTS: 0\n"
        "SCOOTERS: 0\n"
        "TOTAL_PACKAGES: 1\n"
        "SPAWN_FREQUENCY: 1000\n"
    );
    std::string map = makeTempPath("map_spawn");
    writeFile(map,
        "B....\n"
        "..D..\n"
        ".....\n"
    );

    Simulation sim(cfg);
    sim.loadConfig();
    sim.loadMapFromFile(map);
#ifdef UNIT_TEST
    sim.setCurrentTickForTest(50);
    sim.seedRngForTest(12345);
    sim.callSpawnPackageForTest();
    auto &pkgs = sim.getPackagesForTest();
    ASSERT(pkgs.size() == 1);
    int dl = pkgs.back()->getDeadline();
    ASSERT(dl >= 60 && dl <= 70);
#else
    (void)sim;
#endif
    return true;
}

bool test_hungarian_assigns_package_basic() {
    std::string cfg = makeTempPath("cfg_assign");
    writeFile(cfg,
        "MAP_SIZE: 5 5\n"
        "MAX_TICKS: 100\n"
        "DRONES: 1\n"
        "ROBOTS: 0\n"
        "SCOOTERS: 0\n"
        "TOTAL_PACKAGES: 1\n"
        "SPAWN_FREQUENCY: 1000\n"
    );
    std::string map = makeTempPath("map_assign");
    writeFile(map,
        "B....\n"
        "..D..\n"
        ".....\n"
    );

    Simulation sim(cfg);
    sim.loadConfig();
    sim.loadMapFromFile(map);
#ifdef UNIT_TEST
    sim.callSpawnCouriersForTest();
    sim.setCurrentTickForTest(0);
    sim.seedRngForTest(1);
    sim.callSpawnPackageForTest();
    auto &pool = sim.getPackagePoolForTest();
    ASSERT(pool.size() == 1);
    sim.callHiveMindDispatchForTest();
    auto &pool2 = sim.getPackagePoolForTest();
    ASSERT(pool2.size() == 0); // assigned
    auto &couriers = sim.getCouriersForTest();
    ASSERT(!couriers.empty());
    ASSERT(!couriers[0]->getPackages().empty());
#else
    (void)sim;
#endif
    return true;
}

bool test_battery_death() {
    std::string cfg = makeTempPath("cfg_battery");
    writeFile(cfg,
        "MAP_SIZE: 3 3\n"
        "MAX_TICKS: 10\n"
        "DRONES: 1\n"
        "ROBOTS: 0\n"
        "SCOOTERS: 0\n"
        "TOTAL_PACKAGES: 1\n"
        "SPAWN_FREQUENCY: 1000\n"
    );
    std::string map = makeTempPath("map_battery");
    // base at (1,1), dest at (1,2)
    writeFile(map,
        "...\n"
        ".B.\n"
        ".D.\n"
    );

    Simulation sim(cfg);
    sim.loadConfig();
    sim.loadMapFromFile(map);
#ifdef UNIT_TEST
    sim.callSpawnCouriersForTest();
    auto &couriers = sim.getCouriersForTest();
    ASSERT(!couriers.empty());
    // set courier battery to exactly consumption so a single move drops it to 0
    int cons = couriers[0]->getConsumption();
    couriers[0]->setBatteryForTest(cons);

    sim.callSpawnPackageForTest();
    sim.callHiveMindDispatchForTest();
    ASSERT(!couriers[0]->getPackages().empty());

    // advance one step: courier should move and battery becomes 0 -> killed
    sim.callStepForTest();
    ASSERT(couriers[0]->isDead());
    ASSERT(sim.getDeadAgentsForTest() == 1);
#else
    (void)sim;
#endif
    return true;
}

bool test_singleton_enforcement() {
    std::string cfg = makeTempPath("cfg_singleton");
    writeFile(cfg, "MAP_SIZE: 3 3\nMAX_TICKS: 10\nTOTAL_PACKAGES: 0\nSPAWN_FREQUENCY:0\n");
    // first instance
    {
        Simulation sim1(cfg);
        ASSERT(Simulation::hasInstance());
        ASSERT(Simulation::getInstancePtr() == &sim1);
        // second should throw
        bool threw = false;
        try {
            Simulation sim2(cfg);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        ASSERT(threw);
    }
    ASSERT(!Simulation::hasInstance());
    return true;
}

int main() {
    struct Test { const char *name; bool (*fn)(); };
    Test tests[] = {
        {"findPath_flying_and_blocked", test_findPath_flying_and_blocked},
        {"spawnPackage_deadline_relative", test_spawnPackage_deadline_relative},
        {"hungarian_assigns_package_basic", test_hungarian_assigns_package_basic},
        {"singleton_enforcement", test_singleton_enforcement},
    };

    int failed = 0;
    for (auto &t : tests) {
        bool ok = t.fn();
        std::cout << t.name << ": " << (ok ? "OK" : "FAIL") << "\n";
        if (!ok) ++failed;
    }
    if (failed) {
        std::cerr << failed << " tests failed\n";
        return 1;
    }
    std::cout << "All tests passed\n";
    return 0;
}
