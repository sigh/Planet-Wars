#include "Planet.h"
#include "Map.h"

#include <algorithm>

Planet::Planet(
    int planet_id,
    int owner,
    int num_ships
) {
    planet_id_ = planet_id;
    owner_ = owner;
    num_ships_ = num_ships;
    update_prediction_ = true;
    incoming_fleets_.clear();
    prediction_.clear();
}

int Planet::PlanetID() const {
    return planet_id_;
}

int Planet::Owner() const {
    return owner_;
}

int Planet::Ships() const {
    return num_ships_;
}

int Planet::TotalShips(int player_id) const {
    int num_ships = IncomingShips(player_id);

    if ( owner_ == player_id ) {
        num_ships += num_ships_;
    }

    return num_ships;
}

int Planet::IncomingShips(int player_id) const {
    int num_ships = 0;

    for ( int i=0; i < incoming_fleets_.size(); ++i ) {
        num_ships += player_id == 1 
            ? incoming_fleets_[i].first
            : incoming_fleets_[i].second;
    }

    return num_ships;
}

int Planet::WeightedIncoming() const {
    int ships = 0;
    for ( int i=0; i < incoming_fleets_.size(); ++i ) {
        ships += incoming_fleets_[i].first - incoming_fleets_[i].second;
    }
    return ships;
}

void Planet::AddShips(int amount) {
    num_ships_ += amount;
    update_prediction_ = true;
}

void Planet::RemoveShips(int amount) {
    // TODO: WTF
    if ( amount < 0 ) {
        return;
    }
    num_ships_ -= amount;
    if ( num_ships_ < 0 ) {
        num_ships_ = 0;
    }
    update_prediction_ = true;
}

void Planet::AddIncomingFleet(const Fleet &f, int delay) {
    int arrival = f.remaining + delay;

    // ensure incoming_fleets_ is long enough
    if ( arrival + 1 > incoming_fleets_.size() ) {
        incoming_fleets_.resize( arrival + 1, FleetSummary(0,0) );
    }

    // update fleet numbers
    if ( f.owner == 1 ) {
        incoming_fleets_[arrival].first += f.ships;
    }
    else {
        incoming_fleets_[arrival].second += f.ships;
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
            state.ships += Map::GrowthRate(planet_id_) * ( days - (prediction_.size() - 1) );
        }
        return state;
    }
}

// TODO: Make this work for any player anytime
// Currently assumes attacker is playr 1 and
// Future owner is player 2
int Planet::Cost( int days ) const {
    if ( days >= prediction_.size() ) {
        PlanetState state = FutureState(days);
        return state.owner == ME ? 0 : state.ships + 1;
    }

    int ships_delta = 0;
    int required_ships = 0;
    int growth_rate = Map::GrowthRate( planet_id_ );
    int prev_owner = days == 0 ? owner_ : prediction_[days-1].owner;

    if ( prediction_[days].owner == ME )  {
        ships_delta = prediction_[days].ships;
    }
    else if ( prediction_[days].owner == NEUTRAL ) {
        // before anything else we need to ships to overcome the existing force
        required_ships = prediction_[days].ships + 1;

        const FleetSummary &f = incoming_fleets_[days];
        if ( f.first < f.second ) {
            // If there is another attacking force (larger than us)
            // we need to overcome that one too
            required_ships += f.second - f.first;
        }
    }
    else if ( prev_owner == NEUTRAL ) {
        // opponent just took neutral with this move
        const FleetSummary &f = incoming_fleets_[days];
        required_ships = f.second - f.first + 1;
    }
    else {
        required_ships = prediction_[days].ships + 1;
    }

    // TODO: Merge with required ships
    for ( int i = days+1; i < incoming_fleets_.size(); ++i ) {
        const FleetSummary &f = incoming_fleets_[i];
        ships_delta += growth_rate + f.first - f.second;
        if ( ships_delta < 0 ) {
            required_ships += -ships_delta;
            ships_delta = 0; 
        }
    }

    return required_ships;
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
    int growth_rate = Map::GrowthRate( planet_id_ );
    std::vector<int> sort_states(3,0);

    for ( int day=1; day < incoming_fleets_.size(); ++day ) {
        const FleetSummary &f = incoming_fleets_[day];

        // grow planets which have an owner
        if ( state.owner ) {
            state.ships += growth_rate;
        }

        // FIGHT
        if ( state.owner == ME ) {
            // my planet
            state.ships += f.first - f.second;
            if ( state.ships < 0 ) {
                state.owner = ENEMY;
                state.ships = -state.ships;
            }
        }
        else if ( state.owner == ENEMY ) {
            // enemy planet
            state.ships += f.second - f.first;
            if ( state.ships < 0 ) {
                state.owner = ME;
                state.ships = -state.ships;
            }
        }
        else {
            // neutral planet
            sort_states[0] = state.ships;
            sort_states[1] = f.first;
            sort_states[2] = f.second;
            sort( sort_states.begin(), sort_states.end() );

            // the number of ships left is (the maximum minus the second highest)
            state.ships = sort_states[2] - sort_states[1];
            if ( state.ships > 0 ) {
                // if there was a winner, determine who it was
                if ( sort_states[2] == f.first ) {
                    state.owner = ME;
                }
                else if ( sort_states[2] == f.second ) {
                    state.owner = ENEMY;
                }
            }
        }

        // update the state
        prediction_.push_back( state );
    }

    update_prediction_ = false;
}

// number of ships required by player ME to keep this planet
// TODO: Make it work with any owner
int Planet::RequiredShips() const {
    int ships_delta = 0;
    int required_ships = 0;
    int growth_rate = Map::GrowthRate( planet_id_ );

    for ( int day=1; day < incoming_fleets_.size(); ++day ) {
        const FleetSummary &f = incoming_fleets_[day];
        ships_delta += growth_rate + f.first - f.second;
        if ( ships_delta < 0 ) {
            required_ships += -ships_delta;
            ships_delta = 0; 
        }
    }

    return required_ships;
}


