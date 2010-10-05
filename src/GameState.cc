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

std::vector<PlanetPtr> GameState::Planets() const {
    return planets_;
}

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
