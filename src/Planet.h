#ifndef PLANET_H_
#define PLANET_H_

#include "Fleet.h"
#include "counted_ptr.h"

class Planet;
typedef counted_ptr<Planet> PlanetPtr;

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

    int Owner() const;

    // The number of ships on the planet. This is the "population" of the planet.
    int Ships() const;

    // The number of ships coming to the planet owned by given player
    int IncomingShips(int player_id) const;

    // Try to remove amount ships and return the number of ships actually removed
    int RemoveShips(int amount);

    // The number of ships on the planet OR coming to the planet
    // owned by given player
    int TotalShips(int player_id) const;

    void AddIncomingFleet( const Fleet& f, int delay=0);

    // lock ships onto planet and return the number of ships lockd
    int LockShips( int ships );
    PlanetState FutureState(int days) const;
    int FutureOwner() const;
    int Cost( int days ) const;
    int FutureDays() const;
    int WeightedIncoming() const;

    int RequiredShips() const;
    int EffectiveGrowthRate(int owner) const;

 private:
    int planet_id_;
    int owner_;
    int num_ships_;
    int locked_ships_;

    std::vector<FleetSummary> incoming_fleets_;
    mutable std::vector<PlanetState> prediction_;
    mutable bool update_prediction_;

    void UpdatePrediction() const;

};

#endif
