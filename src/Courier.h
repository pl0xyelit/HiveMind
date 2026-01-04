#pragma once
#include <string>
#include <vector>
#include "Package.h"
struct Vec2 {
    int x, y;
};

class Courier {
public:
    Courier(Vec2 startPos,
          int speedPerTick,
          int maxBattery,
          int consumptionPerTick,
          int costPerTick,
          int capacity);

    virtual ~Courier() = default;

    virtual bool canFly() const = 0;
    virtual std::string typeName() const = 0;

    // virtual Vec2 computeNextMove() = 0;

    // Apply movement after dispatcher approves it
    void applyMove(const Vec2& newPos);

    // Package management
    bool assignPackage(class Package* p);
    bool hasFreeCapacity() const;
    const std::vector<class Package*>& getPackages() const;
    void removePackage(class Package* p);
    void recharge(int amount);
    void kill();
    bool isDead() const;
    // --- Getters ---
    Vec2 getPos() const;
    int getSpeed() const;
    int getBattery() const;
    int getMaxBattery() const;
    int getConsumption() const;
    int getCost() const;
    int getCapacity() const;

protected:
    Vec2 pos;
    int speed;
    int maxBattery;
    int battery;
    int consumption;
    int cost;
    int packageCapacity;
    std::vector<Package*> packages;
    bool dead = false;
};

class Drone : public Courier {
public:
    explicit Drone(Vec2 startPos);

    bool canFly() const override;
    std::string typeName() const override;
    // Vec2 computeNextMove() override;
};

class Robot : public Courier {
public:
    explicit Robot(Vec2 startPos);

    bool canFly() const override;
    std::string typeName() const override;
    // Vec2 computeNextMove() override;
};

class Scooter : public Courier {
public:
    explicit Scooter(Vec2 startPos);

    bool canFly() const override;
    std::string typeName() const override;
    // Vec2 computeNextMove() override;
};
