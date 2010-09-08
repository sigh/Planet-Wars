#include "../PlanetWars.h"
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
    incoming_.clear();
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
}

// compare fleets by distance from planet
struct CompareFleets {
    const PlanetWars *pw_;
    CompareFleets(const PlanetWars *pw) { pw_ = pw; }

    bool operator()(int lhs, int rhs) const {
        return pw_->GetFleet(lhs).TurnsRemaining() < pw_->GetFleet(rhs).TurnsRemaining();
    }
};

void Planet::AddIncomingFleet(int fleet) {
    incoming_.push_back(fleet);
    update_projection_ = true;
}

PlanetState Planet::Projection(int days) {
    UpdateProjection();
    if ( days < projection_.size() ) {
        return projection_[days];     
    }
    else {
        PlanetState state = projection_.back();
        state.ships += growth_rate_ * ( days - projection_.size() );
        return state;
    }
}

void Planet::UpdateProjection() {
    std::vector<PlanetState> projection_;

    // If fleets have not been updated we don't need to do anything
    if ( not update_projection_ ) {
        return;
    }

    // sort fleets
    sort(incoming_.begin(), incoming_.end(), CompareFleets(pw_));

    int day = 0;
    PlanetState state;

    // initialise current day
    state.owner = owner_;
    state.ships = num_ships_;
    projection_.push_back(state);

    for ( int i=0; i < incoming_.size(); i++ ) {
        const Fleet &next_fleet = pw_->GetFleet(incoming_[i]);
        while ( day < next_fleet.TurnsRemaining() ) {
            day++;
            if ( state.owner ) {
                state.ships += growth_rate_;
            }
            projection_.push_back(state);
        }
        if ( next_fleet.Owner() == state.owner ) {
            // player is just adding ships to own planet
            state.ships += next_fleet.NumShips();
        }
        else {
            state.ships -= next_fleet.NumShips();
            if ( state.ships < 0 ) {
                state.ships = -state.ships;
                state.owner = next_fleet.Owner();
            }
            else if ( not state.ships ) {
                state.owner = 0;
            }
        }
    }

    // add final state
    if ( state.owner ) {
        state.ships += growth_rate_;
    }
    projection_.push_back(state);

    update_projection_ = false;
}


