#include "Courier.h"

Actor::Actor(Vec2 startPos,
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

void Actor::applyMove(const Vec2& newPos) {
    pos = newPos;
    battery -= consumption;
    if (battery < 0) battery = 0;
}

Vec2 Actor::getPos() const { return pos; }
int Actor::getSpeed() const { return speed; }
int Actor::getBattery() const { return battery; }
int Actor::getMaxBattery() const { return maxBattery; }
int Actor::getCost() const { return cost; }
int Actor::getCapacity() const { return packageCapacity; }

// ---------------- Drone ----------------

Drone::Drone(Vec2 startPos)
    : Actor(startPos,
            /*speed*/ 3,
            /*maxBattery*/ 100,
            /*consumption*/ 10,
            /*cost*/ 15,
            /*capacity*/ 1)
{}

bool Drone::canFly() const { return true; }
std::string Drone::typeName() const { return "Drone"; }

Vec2 Drone::computeNextMove() {
    // TODO: implement proper movement
    return {pos.x + speed, pos.y + speed};
}

// ---------------- Robot ----------------

Robot::Robot(Vec2 startPos)
    : Actor(startPos,
            /*speed*/ 1,
            /*maxBattery*/ 300,
            /*consumption*/ 2,
            /*cost*/ 1,
            /*capacity*/ 4)
{}

bool Robot::canFly() const { return false; }
std::string Robot::typeName() const { return "Robot"; }

Vec2 Robot::computeNextMove() {
    // TODO: implement proper movement
    return {pos.x + speed, pos.y};
}

// ---------------- Scooter ----------------

Scooter::Scooter(Vec2 startPos)
    : Actor(startPos,
            /*speed*/ 2,
            /*maxBattery*/ 200,
            /*consumption*/ 5,
            /*cost*/ 4,
            /*capacity*/ 2)
{}

bool Scooter::canFly() const { return false; }
std::string Scooter::typeName() const { return "Scooter"; }

Vec2 Scooter::computeNextMove() {
    // TODO: implement proper movement
    return {pos.x, pos.y - speed};
}
