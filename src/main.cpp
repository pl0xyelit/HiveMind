#include <iostream>
#include <vector>
#include <memory>

// struct Vec2 {
//     int x, y;
// };

// // placeholder

// class Actor {
// public:
//     Actor(int x, int y) : pos{x, y} {}

//     // Hook for your future logic:
//     // Decide where the actor *wants* to move this tick.
//     virtual Vec2 computeNextMove() = 0;

//     // Apply the move after dispatcher approves it.
//     void applyMove(const Vec2& newPos) {
//         pos = newPos;
//     }

//     Vec2 getPos() const { return pos; }

// protected:
//     Vec2 pos;
// };

// class SimpleActor : public Actor {
// public:
//     using Actor::Actor;

//     // For now, move right every tick.
//     Vec2 computeNextMove() override {
//         return {pos.x + 1, pos.y};
//     }
// };

// class Simulation {
// public:
//     Simulation(int M, int N) : rows(M), cols(N) {}

//     void addActor(std::unique_ptr<Actor> actor) {
//         actors.push_back(std::move(actor));
//     }

//     void tick() {
//         // 1. Ask each actor for its intended move
//         std::vector<Vec2> intendedMoves;
//         intendedMoves.reserve(actors.size());

//         for (auto& actor : actors) {
//             intendedMoves.push_back(actor->computeNextMove());
//         }

//         // 2. Resolve moves (bounds checking, collisions, etc.)
//         for (auto& move : intendedMoves) {
//             if (move.x < 0) move.x = 0;
//             if (move.y < 0) move.y = 0;
//             if (move.x >= cols) move.x = cols - 1;
//             if (move.y >= rows) move.y = rows - 1;
//         }

//         // TODO: collision resolution logic goes here

//         // 3. Apply moves
//         for (size_t i = 0; i < actors.size(); ++i) {
//             actors[i]->applyMove(intendedMoves[i]);
//         }
//     }

//     void printState() const {
//         for (size_t i = 0; i < actors.size(); ++i) {
//             auto p = actors[i]->getPos();
//             std::cout << "Actor " << i << " at (" << p.x << ", " << p.y << ")\n";
//         }
//     }

// private:
//     int rows, cols;
//     std::vector<std::unique_ptr<Actor>> actors;
// };

// int main() {
//     Simulation sim(10, 10);

//     sim.addActor(std::make_unique<SimpleActor>(0, 0));
//     sim.addActor(std::make_unique<SimpleActor>(2, 3));

//     for (int i = 0; i < 5; ++i) {
//         sim.tick();
//         sim.printState();
//         std::cout << "---\n";
//     }
// }
