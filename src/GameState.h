// This file contains helper code that does all the boring stuff for you.
// The code in this file takes care of storing lists of planets and fleets, as
// well as communicating with the game engine. You can get along just fine
// without ever looking at this file. However, you are welcome to modify it
// if you want to.
#ifndef GAME_STATE_H_
#define GAME_STATE_H_

#include <string>
#include <vector>

#include "Planet.h"
#include "Fleet.h"

class GameState {
    public:
        GameState(int turn, std::vector<PlanetPtr>& planets);
        GameState(const GameState& state);

        // return the turn number that we are on
        int Turn() const;

        // Returns the planet with the given planet_id. There are NumPlanets()
        // planets. They are numbered starting at 0.
        PlanetPtr& Planet(int planet_id);
        const PlanetPtr& Planet(int planet_id) const;

        // Returns a list of all the planets.
        std::vector<PlanetPtr> Planets() const;

        std::vector<PlanetPtr> PlanetsOwnedBy(int player) const;
        std::vector<PlanetPtr> PlanetsNotOwnedBy(int player) const;

        // Return a list of the currently pending orders 
        std::vector<Fleet> Orders() const;

        void IssueOrder(const Fleet& order);
        void AddFleet(const Fleet& order);

        // Returns the number of ships that the given player has, either located
        // on planets or in flight.
        int Ships(int player_id) const;

        // The total growth of all planets owned by a player
        int Production(int player_id) const;

        GameState& operator=(const GameState& state);

    private:
        // Store all the planets and fleets. OMG we wouldn't wanna lose all the
        // planets and fleets, would we!?
        std::vector<PlanetPtr> planets_;
        std::vector<Fleet> orders_;
        int turn_;

        void CopyPlanets(const std::vector<PlanetPtr>& planets);
};

#endif

