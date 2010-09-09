#include "Planet.h"

#include <algorithm>

Planet::Planet(
    const PlanetWars* pw,
    int planet_id,
    int owner,
    int num_ships,
    int growth_rate,
    double x,
    double y
) {
    planet_id_ = planet_id;
    owner_ = owner;
    num_ships_ = num_ships;
    growth_rate_ = growth_rate;
    x_ = x;
    y_ = y;
    pw_ = pw;
    update_projection_ = true;
}

int Planet::PlanetID() const {
    return planet_id_;
}

int Planet::Owner() const {
    return owner_;
}

int Planet::NumShips() const {
    return num_ships_;
}

int Planet::GrowthRate() const {
    return growth_rate_;
}

double Planet::X() const {
    return x_;
}

double Planet::Y() const {
    return y_;
}

void Planet::Owner(int new_owner) {
    owner_ = new_owner;
}

void Planet::NumShips(int new_num_ships) {
    num_ships_ = new_num_ships;
}

void Planet::AddShips(int amount) {
    num_ships_ += amount;
}

void Planet::RemoveShips(int amount) {
    num_ships_ -= amount;
    if ( num_ships_ < 0 ) {
        num_ships_ = 0;
    }
}

// compare fleets by distance from planet
struct CompareFleets {
    const PlanetWars *pw_;
    CompareFleets(const PlanetWars *pw) { pw_ = pw; }

    bool operator()(int lhs, int rhs) const {
        return pw_->GetFleet(lhs).TurnsRemaining() < pw_->GetFleet(rhs).TurnsRemaining();
    }
};

void Planet::AddIncomingFleet(const Fleet& fleet) {
    int arrival_day = fleet.TurnsRemaining();

    // ensure incoming_fleets_ is long enough
    if ( arrival_day + 1 > incoming_fleets_.size() ) {
        incoming_fleets_.resize( arrival_day + 1, FleetSummary(0,0) );
    }

    // update fleet numbers
    if ( fleet.Owner() == 1 ) {
        incoming_fleets_[arrival_day].first += fleet.NumShips();
    }
    else {
        incoming_fleets_[arrival_day].second += fleet.NumShips();
    }

    update_projection_ = true;
}

PlanetState Planet::Projection(int days) const {
    UpdateProjection();
    if ( days < projection_.size() ) {
        return projection_[days];     
    }
    else {
        PlanetState state = projection_.back();
        if ( state.owner ) {
            state.ships += growth_rate_ * ( days - (projection_.size() - 1) );
        }
        return state;
    }
}

void Planet::UpdateProjection() const {
    // If fleets have not been updated we don't need to do anything
    if ( not update_projection_ ) {
        return;
    }

    projection_.clear();

    // initialise current day
    PlanetState state;
    state.owner = owner_;
    state.ships = num_ships_;
    projection_.push_back(state);

    for ( int day=1; day < incoming_fleets_.size(); ++day ) {
        const FleetSummary &f = incoming_fleets_[day];

        // grow planets which have an owner
        if ( state.owner ) {
            state.ships += growth_rate_;
        }

        // FIGHT
        if ( state.owner == 1 ) {
            // my planet
            state.ships += f.first - f.second;
            if ( state.ships < 0 ) {
                state.owner = 2;
            }
        }
        else if ( state.owner == 2 ) {
            // enemy planet
            state.ships += f.second - f.first;
            if ( state.ships < 0 ) {
                state.owner = 1;
            }
        }
        else {
            // neutral planet
            int num_ships = f.first + f.second;
            if ( num_ships <= state.ships ) {
                // not enough units to conquer neutral
                state.ships -= num_ships; 
            }
            else if ( f.first <= state.ships/2 ) {
                state.ships -= num_ships;
                state.owner = 2;
            }
            else if ( f.second <= state.ships/2 ) {
                state.ships -= num_ships;
                state.owner = 1;
            }
            else {
                state.ships = f.first - f.second;
                state.owner = state.ships > 0 ? 1 : 2;
            }
        }

        // clean up the mess
        if ( state.ships < 0 ) {
            // catering for bug in game engine
            // TODO: Remove when fixed
            state.ships = -state.ships - 1;
        }
        if ( state.ships == 0 ) {
            // A planet with 0 units is neutral
            state.owner = 0;
        }

        projection_.push_back( state );
    }

    update_projection_ = false;
}


