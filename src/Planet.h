#ifndef PLANET_H_
#define PLANET_H_

#include "PlanetWars.h"

class PlanetWars;
class Fleet;

// my ships, enemy ships
typedef std::pair<int, int> FleetSummary;
// owner, ships
struct PlanetState {
    int owner;
    int ships;
};

// Stores information about one planet. There is one instance of this class
// for each planet on the map.
class Planet {
 public:
    // Initializes a planet.
    Planet( 
        int planet_id,
        int owner,
        int num_ships
    );

    // Returns the ID of this planets. Planets are numbered starting at zero.
    int PlanetID() const;

    // Returns the ID of the player that owns this planet. Your playerID is
    // always 1. If the owner is 1, this is your planet. If the owner is 0, then
    // the planet is neutral. If the owner is 2 or some other number, then this
    // planet belongs to the enemy.
    int Owner() const;

    // The number of ships on the planet. This is the "population" of the planet.
    int Ships() const;

    // The number of ships coming to the planet owned by given player
    int IncomingShips(int player_id) const;

    // The number of ships on the planet OR coming to the planet
    // owned by given player
    int TotalShips(int player_id) const;

    // Use the following functions to set the properties of this planet. Note
    // that these functions only affect your program's copy of the game state.
    // You can't steal your opponent's planets just by changing the owner to 1
    // using the Owner(int) function! :-)
    void AddShips(int amount);
    void RemoveShips(int amount);

    void AddIncomingFleet( const Fleet& f);
    PlanetState FutureState(int days) const;
    int FutureOwner() const;
    int Cost( int days ) const;
    int FutureDays() const;
    int WeightedIncoming() const;

 private:
    int planet_id_;
    int owner_;
    int num_ships_;

    std::vector<FleetSummary> incoming_fleets_;
    mutable std::vector<PlanetState> prediction_;
    mutable bool update_prediction_;

    void UpdatePrediction() const;
};

#endif
