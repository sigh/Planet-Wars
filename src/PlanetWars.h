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

#include <fstream>
extern std::ofstream LOG_FILE;

class Planet;

struct Order {
    public:
        int source;
        int dest;
        int ships;
        Order(int source, int dest, int ships)
            : source(source), dest(dest), ships(ships) {}
};

struct Fleet {
    public:
        int owner;
        int dest;
        int ships;
        int remaining;
        Fleet() {};
        Fleet(int owner, const Order &o) : owner(owner) {
            dest = o.dest;
            ships = o.ships;
            remaining = Map::Distance( o.source, o.dest );
        }
};

class PlanetWars {
    public:
        PlanetWars(std::vector<Planet> planets);

        // Returns the planet with the given planet_id. There are NumPlanets()
        // planets. They are numbered starting at 0.
        Planet& GetPlanet(int planet_id);

        // Returns a list of all the planets.
        std::vector<Planet> Planets() const;

        // Return a list of all the planets owned by the current player. By
        // convention, the current player is always player number 1.
        std::vector<Planet> MyPlanets() const;

        // Return a list of all neutral planets.
        std::vector<Planet> NeutralPlanets() const;

        // Return a list of all the planets owned by rival players. This excludes
        // planets owned by the current player, as well as neutral planets.
        std::vector<Planet> EnemyPlanets() const;

        // Return a list of all the planets that are not owned by the current
        // player. This includes all enemy planets and neutral planets.
        std::vector<Planet> NotMyPlanets() const;

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
        std::vector<Planet> planets_;
        std::vector<Order> orders_;
};

#endif

