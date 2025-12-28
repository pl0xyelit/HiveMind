#pragma once
#include <string>
#include <iostream>

struct Vec2 {
    int x, y;
};

class Actor {
public:
    Actor(Vec2 startPos,
          int speedPerTick,
          int maxBattery,
          int consumptionPerTick,
          int costPerTick,
          int capacity)
        : pos(startPos),
          speed(speedPerTick),
          maxBattery(maxBattery),
          battery(maxBattery),
          consumption(consumptionPerTick),
          cost(costPerTick),
          packageCapacity(capacity)
    {}

    virtual ~Actor() = default;

    // --- Capabilities ---
    virtual bool canFly() const = 0;
    virtual std::string typeName() const = 0;

    // --- Movement logic (can be overridden if needed) ---
    virtual Vec2 computeNextMove() = 0;

    // Apply movement after dispatcher approves it
    void applyMove(const Vec2& newPos) {
        pos = newPos;
        battery -= consumption;
        if (battery < 0) battery = 0;
    }

    // --- Getters ---
    Vec2 getPos() const { return pos; }
    int getSpeed() const { return speed; }
    int getBattery() const { return battery; }
    int getMaxBattery() const { return maxBattery; }
    int getCost() const { return cost; }
    int getCapacity() const { return packageCapacity; }

protected:
    Vec2 pos;
    int speed;
    int maxBattery;
    int battery;
    int consumption;
    int cost;
    int packageCapacity;
};

class Drone : public Actor {
public:
    Drone(Vec2 startPos)
        : Actor(startPos,
                /*speed*/ 2,
                /*maxBattery*/ 100,
                /*consumption*/ 2,
                /*cost*/ 5,
                /*capacity*/ 1)
    {}

    bool canFly() const override { return true; }
    std::string typeName() const override { return "Drone"; }

    Vec2 computeNextMove() override {
        // Placeholder logic: move diagonally
        return {pos.x + speed, pos.y + speed};
    }
};

class Robot : public Actor {
public:
    Robot(Vec2 startPos)
        : Actor(startPos,
                /*speed*/ 1,
                /*maxBattery*/ 200,
                /*consumption*/ 1,
                /*cost*/ 3,
                /*capacity*/ 3)
    {}

    bool canFly() const override { return false; }
    std::string typeName() const override { return "Robot"; }

    Vec2 computeNextMove() override {
        // Placeholder logic: move right
        return {pos.x + speed, pos.y};
    }
};

class Scooter : public Actor {
public:
    Scooter(Vec2 startPos)
        : Actor(startPos,
                /*speed*/ 3,
                /*maxBattery*/ 50,
                /*consumption*/ 3,
                /*cost*/ 2,
                /*capacity*/ 1)
    {}

    bool canFly() const override { return false; }
    std::string typeName() const override { return "Scooter"; }

    Vec2 computeNextMove() override {
        // Placeholder logic: move up
        return {pos.x, pos.y - speed};
    }
};
