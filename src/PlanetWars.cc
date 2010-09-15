#include "PlanetWars.h"
#include <cmath>
#include <string>
#include <vector>
#include <sstream>

PlanetWars::PlanetWars(std::vector<Planet> planets)
    : planets_(planets) { }

Planet& PlanetWars::GetPlanet(int planet_id) {
    return planets_[planet_id];
}

std::vector<Planet> PlanetWars::Planets() const {
    std::vector<Planet> r;
    for (int i = 0; i < planets_.size(); ++i) {
        const Planet& p = planets_[i];
        r.push_back(p);
    }
    return r;
}

std::vector<Planet> PlanetWars::MyPlanets() const {
    std::vector<Planet> r;
    for (int i = 0; i < planets_.size(); ++i) {
        const Planet& p = planets_[i];
        if (p.Owner() == 1) {
            r.push_back(p);
        }
    }
    return r;
}

std::vector<Planet> PlanetWars::NeutralPlanets() const {
    std::vector<Planet> r;
    for (int i = 0; i < planets_.size(); ++i) {
        const Planet& p = planets_[i];
        if (p.Owner() == 0) {
            r.push_back(p);
        }
    }
    return r;
}

std::vector<Planet> PlanetWars::EnemyPlanets() const {
    std::vector<Planet> r;
    for (int i = 0; i < planets_.size(); ++i) {
        const Planet& p = planets_[i];
        if (p.Owner() > 1) {
            r.push_back(p);
        }
    }
    return r;
}

std::vector<Planet> PlanetWars::NotMyPlanets() const {
    std::vector<Planet> r;
    for (int i = 0; i < planets_.size(); ++i) {
        const Planet& p = planets_[i];
        if (p.Owner() != 1) {
            r.push_back(p);
        }
    }
    return r;
}

int PlanetWars::Production(int player_id) const {
    int production = 0;
    for (int i = 0; i < planets_.size(); ++i) {
        if ( planets_[i].Owner() == player_id ) { 
            production += Map::GrowthRate(i);
        }
    }
    return production;
}

int PlanetWars::Ships(int player_id) const {
    int ships = 0;
    for (int i = 0; i < planets_.size(); ++i) {
        ships += planets_[i].TotalShips(player_id);
    }
    return ships;
}

std::vector<Order> PlanetWars::Orders() const {
    return orders_;
}

void PlanetWars::IssueOrder(const Order& order, int delay) {
    planets_[order.source].RemoveShips( order.ships );
    planets_[order.dest].AddIncomingFleet( Fleet( ME, order ), delay );

    // only execute order if it is for now
    if ( delay == 0 ) {
        orders_.push_back(order);
    }
}

