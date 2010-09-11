#include "PlanetWars.h"
#include <cmath>
#include <string>
#include <vector>
#include <sstream>

PlanetWars::PlanetWars(std::vector<Planet> planets, std::vector<Fleet> fleets)
    : planets_(planets), fleets_(fleets)  { }

Planet& PlanetWars::GetPlanet(int planet_id) {
    return planets_[planet_id];
}

int PlanetWars::NumFleets() const {
    return fleets_.size();
}

const Fleet& PlanetWars::GetFleet(int fleet_id) const {
    return fleets_[fleet_id];
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

std::vector<Fleet> PlanetWars::Fleets() const {
    std::vector<Fleet> r;
    for (int i = 0; i < fleets_.size(); ++i) {
        const Fleet& f = fleets_[i];
        r.push_back(f);
    }
    return r;
}

std::vector<Fleet> PlanetWars::MyFleets() const {
    std::vector<Fleet> r;
    for (int i = 0; i < fleets_.size(); ++i) {
        const Fleet& f = fleets_[i];
        if (f.Owner() == 1) {
            r.push_back(f);
        }
    }
    return r;
}

std::vector<Fleet> PlanetWars::EnemyFleets() const {
    std::vector<Fleet> r;
    for (int i = 0; i < fleets_.size(); ++i) {
        const Fleet& f = fleets_[i];
        if (f.Owner() > 1) {
            r.push_back(f);
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
        Fleet(
            1,
            order.ships, 
            order.source,
            order.dest,
            Map::Distance( order.source, order.dest ),
            Map::Distance( order.source, order.dest )
        )
    );
    orders_.push_back(order);
}

bool PlanetWars::IsAlive(int player_id) const {
    for (unsigned int i = 0; i < planets_.size(); ++i) {
        if (planets_[i].Owner() == player_id) {
            return true;
        }
    }
    for (unsigned int i = 0; i < fleets_.size(); ++i) {
        if (fleets_[i].Owner() == player_id) {
            return true;
        }
    }
    return false;
}

