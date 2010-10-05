#ifndef MAP_H_
#define MAP_H_

#include <vector>

namespace Map {
    // Returns the growth rate of the planet. Unless the planet is neutral, the
    // population of the planet grows by this amount each turn. The higher this
    // number is, the faster this planet produces ships.
    int GrowthRate(int planet);

    // Returns the distance between two planets, rounded up to the next highest
    // integer. This is the number of discrete time steps it takes to get between
    // the two planets.
    int Distance(int source, int dest);

    // Returns the number of planets on the map
    int NumPlanets();

    // return a vector of planet ids by distance from the given source
    const std::vector<int>& PlanetsByDistance(int planet);

    // Add a planet to the map
    void AddPlanet(int growth_rate, double x, double y);

    // Initilise data structures in the map
    void Init();
}

#endif
