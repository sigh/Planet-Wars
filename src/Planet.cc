#include "Planet.h"
#include "Map.h"
#include "Log.h"

#include <algorithm>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

Planet::Planet( int planet_id, Player owner, int num_ships):
    id(planet_id),
    owner_(owner),
    num_ships_(num_ships),
    locked_ships_(0),
    growth_rate_(Map::GrowthRate(id)),
    effective_growth_rate_(growth_rate_),
    incoming_fleets_(1,FleetSummary()),
    update_prediction_(true) { 

}
    
Player Planet::Owner() const {
    return owner_;
}

int Planet::Ships(bool locked /* = false */) const {
    return locked ? std::max(num_ships_ - locked_ships_,0) : num_ships_;
}

int Planet::TotalShips(Player player) const {
    int num_ships = IncomingShips(player);

    if ( owner_ == player ) {
        num_ships += num_ships_;
    }

    return num_ships;
}

int Planet::IncomingShips(Player player) const {
    int num_ships = 0;

    foreach ( const FleetSummary& f, incoming_fleets_ ) {
        num_ships += f[player];
    }

    return num_ships;
}

int Planet::WeightedIncoming(Player player, unsigned int days) const {
    int ships = 0;
    if ( days > incoming_fleets_.size() ) {
        days = incoming_fleets_.size();
    }
    for ( unsigned int i=0; i < days; ++i ) {
        ships += incoming_fleets_[i].delta(player);
    }
    return ships;
}

int Planet::RemoveShips(int amount) {
    if ( amount < 0 ) {
        LOG_ERROR("Trying to remove " << amount << " ships from " << id );
        return 0;
    }
    num_ships_ -= amount;
    if ( num_ships_ < 0 ) {
        amount += num_ships_;
        num_ships_ = 0;
    }
    update_prediction_ = true;

    return amount;
}

void Planet::AddIncomingFleet(const Fleet &f) {
    unsigned int arrival = f.remaining();

    // ensure incoming_fleets_ is long enough
    if ( arrival + 1 > incoming_fleets_.size() ) {
        incoming_fleets_.resize( arrival + 1, FleetSummary() );
    }

    incoming_fleets_[arrival][f.owner] += f.ships;

    update_prediction_ = true;
}

int Planet::GrowthRate() const {
    return growth_rate_;
}

int Planet::LockShips(int ships) {
    locked_ships_ += ships;
    LOG( " Locked on " << id << ": " << locked_ships_);
    if ( locked_ships_ > num_ships_ ) {
        ships -= locked_ships_ - num_ships_;
        locked_ships_ = num_ships_;
    }
    return ships;
}

// predicted owner after all fleets have arrived
PlanetState Planet::FutureState(unsigned int days) const {
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
int Planet::Cost( unsigned int days, Player attacker ) const {
    Player defender = -attacker;

    if ( days >= prediction_.size() ) {
        PlanetState state = FutureState(days);
        return state.owner == attacker ? 0 : state.ships + 1;
    }

    int ships_delta = 0;
    int required_ships = 0;
    int prev_owner = days == 0 ? owner_ : prediction_[days-1].owner;

    if ( prediction_[days].owner == attacker )  {
        ships_delta = prediction_[days].ships;
    }
    else if ( prediction_[days].owner == NEUTRAL ) {
        // before anything else we need to ships to overcome the existing force
        required_ships = prediction_[days].ships + 1;

        const FleetSummary &f = incoming_fleets_[days];
        if ( f[attacker] < f[defender] ) {
            // If there is another attacking force (larger than us)
            // we need to overcome that one too
            required_ships += f.delta(defender);
        }
    }
    else if ( prev_owner == NEUTRAL ) {
        // opponent just took neutral with this move
        const FleetSummary &f = incoming_fleets_[days];
        required_ships = f.delta(defender) + 1;
    }
    else {
        required_ships = prediction_[days].ships + 1;
    }

    // TODO: Merge with required ships
    for ( unsigned int i = days+1; i < incoming_fleets_.size(); ++i ) {
        const FleetSummary &f = incoming_fleets_[i];
        ships_delta += growth_rate_ + f.delta(attacker);
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
    state.ships = num_ships_ + incoming_fleets_[0][owner_];
    prediction_.push_back(state);
    std::vector<int> sort_states(3,0);

    for ( unsigned int day=1; day < incoming_fleets_.size(); ++day ) {
        const FleetSummary &f = incoming_fleets_[day];

        // grow planets which have an owner
        if ( state.owner != NEUTRAL ) {
            state.ships += growth_rate_;
        }

        if ( ! f.empty() ) {
            // There are ships: FIGHT

            if ( state.owner != NEUTRAL ) {
                // occupied planet
                state.ships += f.delta(state.owner);
                if ( state.ships < 0 ) {
                    state.owner = -state.owner;
                    state.ships = -state.ships;
                }
            }
            else {
                // neutral planet
                sort_states[0] = state.ships;
                sort_states[1] = f[ME];
                sort_states[2] = f[ENEMY];
                sort( sort_states.begin(), sort_states.end() );

                // the number of ships left is (the maximum minus the second highest)
                state.ships = sort_states[2] - sort_states[1];
                if ( state.ships > 0 ) {
                    // if there was a winner, determine who it was
                    if ( sort_states[2] == f[ME] ) {
                        state.owner = ME;
                    }
                    else if ( sort_states[2] == f[ENEMY] ) {
                        state.owner = ENEMY;
                    }
                }
            }
        }

        // update the state
        prediction_.push_back( state );
    }

    update_prediction_ = false;
}

// number of ships required by current owner to keep this planet
int Planet::RequiredShips() const {
    int ships_delta = 0;
    int required_ships = 0;

    for ( unsigned int day=1; day < incoming_fleets_.size(); ++day ) {
        const FleetSummary &f = incoming_fleets_[day];
        ships_delta += growth_rate_ + f.delta(owner_);
        if ( ships_delta < 0 ) {
            required_ships += -ships_delta;
            ships_delta = 0; 
        }
    }

    return required_ships;
}

// number of extra ships available to the current owner of this planet
int Planet::ShipExcess(unsigned int days) const {
    int ships_delta = 0;
    int required_ships = 0;

    unsigned int day = 1;
    for ( ; day < incoming_fleets_.size() && day < days; ++day ) {
        const FleetSummary &f = incoming_fleets_[day];
        ships_delta += growth_rate_ + f.delta(owner_);
        if ( ships_delta < 0 ) {
            required_ships += -ships_delta;
            ships_delta = 0; 
        }
    }

    for ( ; day < days; ++day ) {
        ships_delta += growth_rate_;
    }

    return ships_delta;
}

PlanetPtr Planet::Clone() const {
    return PlanetPtr(new Planet(*this));
}

