#include <cmath>
#include <string>
#include <vector>
#include <sstream>

#include "GameState.h"
#include "Log.h"
#include "Config.h"
#include "Map.h"

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

GameState::GameState(int turn, std::vector<PlanetPtr>& planets)
    : planets_(planets), turn_(turn) { }

GameState::GameState(const GameState& state)
    : orders_(state.orders_), turn_(state.turn_) {
    CopyPlanets(state.planets_);
}

int GameState::Turn() const {
    return turn_;
}

PlanetPtr& GameState::Planet(int planet_id) {
    return planets_[planet_id];
}
const PlanetPtr& GameState::Planet(int planet_id) const {
    return planets_[planet_id];
}

std::vector<PlanetPtr> GameState::Planets() {
    return planets_;
}

// WTF: this is const but lets us return non-const PlanetPtr
//      I tried to overload but couldn't
//      Either me fail or C++ HUGE fail.
std::vector<PlanetPtr> GameState::PlanetsOwnedBy(Player player) const {
    std::vector<PlanetPtr> r;
    foreach ( const PlanetPtr& p, planets_ )  {
        if (p->Owner() == player) {
            r.push_back(p);
        }
    }
    return r;
}

std::vector<PlanetPtr> GameState::PlanetsNotOwnedBy(Player player) const {
    std::vector<PlanetPtr> r;
    foreach ( const PlanetPtr& p, planets_ )  {
        if (p->Owner() != player) {
            r.push_back(p);
        }
    }
    return r;
}

int GameState::Production(Player player) const {
    int production = 0;
    foreach ( const PlanetPtr& p, planets_ )  {
        if ( p->Owner() == player ) { 
            production += p->GrowthRate();
        }
    }
    return production;
}

int GameState::Ships(Player player) const {
    int ships = 0;
    foreach ( const PlanetPtr& p, planets_ )  {
        ships += p->TotalShips(player);
    }
    return ships;
}

std::vector<Fleet> GameState::Orders() const {
    return orders_;
}

void GameState::IssueOrder(const Fleet& order) {
    AddFleet(order);

    // only store order if it is for now
    if ( order.launch == 0 ) {
        orders_.push_back(order);
    }
    else {
        LOG( "  DELAYED ORDER: " << order.source << " " << order.dest << " " << order.ships << " | " << order.launch );
    }
}

void GameState::AddFleet(const Fleet& order) {
    if ( order.launch >= 0 ) {
        planets_[order.source]->RemoveShips( order.ships );
    }

    planets_[order.dest]->AddIncomingFleet( order );
}

GameState& GameState::operator=(const GameState& state) {
    if ( this != &state ) {
        // Copy the orders and the turns
        orders_ = state.orders_;
        turn_ = state.turn_;

        // we want to CLONE the planets because they are pointers
        CopyPlanets(state.planets_);
    }

    return *this;
}

void GameState::CopyPlanets(const std::vector<PlanetPtr>& planets) {
    planets_.clear();
    foreach ( const PlanetPtr& p, planets ) {
        planets_.push_back(p->Clone());
    }
}

PlanetPtr GameState::ClosestPlanetByOwner(PlanetPtr planet, Player player) const {
    const std::vector<int>& sorted = Map::PlanetsByDistance( planet->id );

    foreach ( int p, sorted ) {
        if ( p == planet->id ) continue;
        if ( planets_[p]->Owner() == player ) {
            return planets_[p];
        }
    }

    return PlanetPtr();
}

int GameState::ShipsWithinRange(PlanetPtr planet, int distance, Player owner) const {
    const std::vector<int>& sorted = Map::PlanetsByDistance(planet->id);

    int ships = 0;

    foreach ( int i, sorted ) {
        if ( i == planet->id ) continue;
        int helper_distance = Map::Distance(planet->id, i);
        if ( helper_distance >= distance ) break;

        if ( planets_[i]->Owner() != owner ) continue;
        ships += planets_[i]->Ships();
    }
    
    return ships;
}

// Find planets closest to the opponent
std::map<int,bool> GameState::FrontierPlanets(Player player) const {
    // TODO: use bitmap
    std::map<int,bool> frontier_planets;
    std::vector<PlanetPtr> opponent_planets = PlanetsOwnedBy(-player);

    foreach ( const PlanetPtr& p, opponent_planets ) {
        PlanetPtr closest = ClosestPlanetByOwner(p,player);
        if ( closest ) {
            frontier_planets[closest->id] = true;
        }
    }
    return frontier_planets;
}

// Find planets that will be closest to the opponent
std::map<int,bool> GameState::FutureFrontierPlanets(Player player) const {
    std::map<int,bool> frontier_planets;

    std::vector<PlanetPtr> opponent_planets = PlanetsOwnedBy(-player);
    foreach ( const PlanetPtr& opponent_planet, opponent_planets ) {
        const std::vector<int>& sorted = Map::PlanetsByDistance( opponent_planet->id );
        PlanetPtr closest;
        foreach ( int p, sorted ) {
            if ( p == opponent_planet->id ) continue;
            if ( planets_[p]->FutureOwner() == player ) {
                closest = planets_[p];
                break;
            }
        }
        if ( closest ) {
            frontier_planets[closest->id] = true;
        }
    }

    return frontier_planets;
}
