#include "Planet.h"

#include <algorithm>

Planet::Planet(
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
    update_prediction_ = true;
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

    update_prediction_ = true;
}

// predicted owner after all fleets have arrived
PlanetState Planet::FutureState(int days) const {
    UpdatePrediction();
    if ( days < prediction_.size() ) {
        return prediction_[days];     
    }
    else {
        PlanetState state = prediction_.back();
        if ( state.owner ) {
            state.ships += growth_rate_ * ( days - (prediction_.size() - 1) );
        }
        return state;
    }
}

// TODO: Make this work for any player anytime
// Currently assumes attacker is playr 1 and
// Future owner is player 2
int Planet::Cost( int days ) const {
    if ( days >= prediction_.size() ) {
        return FutureState( days ).ships; 
    }

    int ships_required = prediction_[days].ships;
    for ( int i = days+1; i < incoming_fleets_.size(); ++i ) {
        ships_required += incoming_fleets_[i].second - incoming_fleets_[i].first;
    }

}

// return the number of days until the last fleet has arrived
int Planet::FutureDays() const {
    UpdatePrediction();
    return prediction_.size() - 1;
}

// predicted owner after all fleets have arrived
int Planet::FutureOwner() const {
    UpdatePrediction();
    return prediction_.back().owner;
}

void Planet::UpdatePrediction() const {
    // If fleets have not been updated we don't need to do anything
    if ( not update_prediction_ ) {
        return;
    }

    prediction_.clear();

    // initialise current day
    PlanetState state;
    state.owner = owner_;
    state.ships = num_ships_;
    prediction_.push_back(state);

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

        prediction_.push_back( state );
    }

    update_prediction_ = false;
}


