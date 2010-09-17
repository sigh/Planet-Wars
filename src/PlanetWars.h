// This file contains helper code that does all the boring stuff for you.
// The code in this file takes care of storing lists of planets and fleets, as
// well as communicating with the game engine. You can get along just fine
// without ever looking at this file. However, you are welcome to modify it
// if you want to.
#ifndef PLANET_WARS_H_
#define PLANET_WARS_H_

#include <string>
#include <vector>

#include "Map.h"
#include "Planet.h"
#include "Config.h"
#include "Fleet.h"

class PlanetWars {
    public:
        PlanetWars(std::vector<PlanetPtr> planets);

        // Returns the planet with the given planet_id. There are NumPlanets()
        // planets. They are numbered starting at 0.
        PlanetPtr GetPlanet(int planet_id);
        const PlanetPtr GetPlanet(int planet_id) const;

        // Returns a list of all the planets.
        std::vector<PlanetPtr> Planets() const;

        std::vector<PlanetPtr> PlanetsOwnedBy(int player) const;
        std::vector<PlanetPtr> PlanetsNotOwnedBy(int player) const;

        // Return a list of the currently pending orders 
        std::vector<Order> Orders() const;

        // Sends an order to the game engine. The order is to send num_ships ships
        // from source_planet to destination_planet. The order must be valid, or
        // else your bot will get kicked and lose the game. For example, you must own
        // source_planet, and you can't send more ships than you actually have on
        // that planet.
        void IssueOrder(const Order& order);

        // Returns the number of ships that the given player has, either located
        // on planets or in flight.
        int Ships(int player_id) const;

        // The total growth of all planets owned by a player
        int Production(int player_id) const;

    private:
        // Store all the planets and fleets. OMG we wouldn't wanna lose all the
        // planets and fleets, would we!?
        std::vector<PlanetPtr> planets_;
        std::vector<Order> orders_;
};

#endif

