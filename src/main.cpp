#include <iostream>
#include "Simulation.h"

int main() {
    Simulation sim("simulation_setup.txt");
    sim.run();
    std::cout << "Simulation finished. See simulation.txt\n";
    return 0;
}
