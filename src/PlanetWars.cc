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

std::vector<Order> PlanetWars::Orders() const {
    return orders_;
}

void PlanetWars::IssueOrder(Order order) {
    planets_[order.source].RemoveShips( order.ships );
    planets_[order.dest].AddIncomingFleet(
        1,
        order.dest,
        order.ships, 
        Map::Distance( order.source, order.dest )
    );
    orders_.push_back(order);
}

