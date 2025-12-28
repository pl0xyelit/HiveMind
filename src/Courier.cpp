#include "Courier.h"

Courier::Courier(Vec2 startPos,
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
{
    // packages vector starts empty; use assignPackage to add
}

void Courier::applyMove(const Vec2& newPos) {
    pos = newPos;
    battery -= consumption;
    if (battery < 0) battery = 0;
}

Vec2 Courier::getPos() const { return pos; }
int Courier::getSpeed() const { return speed; }
int Courier::getBattery() const { return battery; }
int Courier::getMaxBattery() const { return maxBattery; }
int Courier::getConsumption() const { return consumption; }
int Courier::getCost() const { return cost; }
int Courier::getCapacity() const { return packageCapacity; }

bool Courier::assignPackage(Package* p) {
    if ((int)packages.size() < packageCapacity) {
        packages.push_back(p);
        return true;
    }
    return false;
}

bool Courier::hasFreeCapacity() const {
    return (int)packages.size() < packageCapacity;
}

const std::vector<Package*>& Courier::getPackages() const {
    return packages;
}

void Courier::removePackage(Package* p) {
    for (auto it = packages.begin(); it != packages.end(); ++it) {
        if (*it == p) { packages.erase(it); return; }
    }
}

void Courier::recharge(int amount) {
    battery += amount;
    if (battery > maxBattery) battery = maxBattery;
}

void Courier::kill() {
    dead = true;
    speed = 0;
    battery = 0;
}

bool Courier::isDead() const {
    return dead;
}

// ---------------- Drone ----------------

Drone::Drone(Vec2 startPos)
    : Courier(startPos,
            /*speed*/ 3,
            /*maxBattery*/ 100,
            /*consumption*/ 10,
            /*cost*/ 15,
            /*capacity*/ 1)
{
    // no-op: leave packages empty and reserve capacity if needed
    packages.reserve(packageCapacity);

}

bool Drone::canFly() const { return true; }
std::string Drone::typeName() const { return "Drone"; }

Vec2 Drone::computeNextMove() {
    // TODO: implement proper movement
    return {pos.x + speed, pos.y + speed};
}

// ---------------- Robot ----------------

Robot::Robot(Vec2 startPos)
    : Courier(startPos,
            /*speed*/ 1,
            /*maxBattery*/ 300,
            /*consumption*/ 2,
            /*cost*/ 1,
            /*capacity*/ 4)
{
    // no-op: leave packages empty and reserve capacity if needed
    packages.reserve(packageCapacity);
}

bool Robot::canFly() const { return false; }
std::string Robot::typeName() const { return "Robot"; }

Vec2 Robot::computeNextMove() {
    // TODO: implement proper movement
    return {pos.x + speed, pos.y};
}

// ---------------- Scooter ----------------

Scooter::Scooter(Vec2 startPos)
    : Courier(startPos,
            /*speed*/ 2,
            /*maxBattery*/ 200,
            /*consumption*/ 5,
            /*cost*/ 4,
            /*capacity*/ 2)
{
    // no-op: leave packages empty and reserve capacity if needed
    packages.reserve(packageCapacity);
}

bool Scooter::canFly() const { return false; }
std::string Scooter::typeName() const { return "Scooter"; }

Vec2 Scooter::computeNextMove() {
    // TODO: implement proper movement
    return {pos.x, pos.y - speed};
}
