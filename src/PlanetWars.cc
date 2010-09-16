#include "PlanetWars.h"
#include <cmath>
#include <string>
#include <vector>
#include <sstream>

PlanetWars::PlanetWars(std::vector<PlanetPtr> planets)
    : planets_(planets) { }

PlanetPtr PlanetWars::GetPlanet(int planet_id) {
    return planets_[planet_id];
}

std::vector<PlanetPtr> PlanetWars::Planets() const {
    return planets_;
}

std::vector<PlanetPtr> PlanetWars::PlanetsOwnedBy(int player) const {
    std::vector<PlanetPtr> r;
    for (int i = 0; i < planets_.size(); ++i) {
        PlanetPtr p = planets_[i];
        if (p->Owner() == player) {
            r.push_back(p);
        }
    }
    return r;
}

std::vector<PlanetPtr> PlanetWars::PlanetsNotOwnedBy(int player) const {
    std::vector<PlanetPtr> r;
    for (int i = 0; i < planets_.size(); ++i) {
        PlanetPtr p = planets_[i];
        if (p->Owner() != player) {
            r.push_back(p);
        }
    }
    return r;
}

int PlanetWars::Production(int player_id) const {
    int production = 0;
    for (int i = 0; i < planets_.size(); ++i) {
        if ( planets_[i]->Owner() == player_id ) { 
            production += Map::GrowthRate(i);
        }
    }
    return production;
}

int PlanetWars::Ships(int player_id) const {
    int ships = 0;
    for (int i = 0; i < planets_.size(); ++i) {
        ships += planets_[i]->TotalShips(player_id);
    }
    return ships;
}

std::vector<Order> PlanetWars::Orders() const {
    return orders_;
}

void PlanetWars::IssueOrder(const Order& order, int delay) {
    planets_[order.source]->RemoveShips( order.ships );
    planets_[order.dest]->AddIncomingFleet( Fleet( ME, order ), delay );

    // only execute order if it is for now
    if ( delay == 0 ) {
        orders_.push_back(order);
    }
}

