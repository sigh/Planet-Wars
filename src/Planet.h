#ifndef PLANET_H_
#define PLANET_H_

#include "Fleet.h"
#include "Player.h"
#include <boost/shared_ptr.hpp>

const int INF = 999999;

class Planet;
typedef boost::shared_ptr<Planet> PlanetPtr;

// owner, ships
struct PlanetState {
    Player owner;
    int ships;
};

// Stores information about one planet. There is one instance of this class
// for each planet on the map.
class Planet {
 public:
    // Initializes a planet.
    Planet( 
        int planet_id,
        Player owner,
        int num_ships
    );

    // id is a such a basic item that it is public
    const int id;

    Player Owner() const;

    // The number of ships on the planet. This is the "population" of the planet.
    int Ships(bool locked=false) const;

    // The number of ships coming to the planet owned by given player
    int IncomingShips(Player player) const;

    // Try to remove amount ships and return the number of ships actually removed
    int RemoveShips(int amount);

    // The number of ships on the planet OR coming to the planet
    // owned by given player
    int TotalShips(Player player) const;

    void AddIncomingFleet(const Fleet& f);

    // lock ships onto planet and return the number of ships lockd
    int LockShips( int ships );
    PlanetState FutureState(unsigned int days) const;
    int FutureOwner() const;
    int Cost( unsigned int days, Player attacker ) const;
    int FutureDays() const;
    int WeightedIncoming(Player player, unsigned int days=INF) const;

    int RequiredShips() const;
    int ShipExcess(unsigned int days) const;

    int GrowthRate() const;

    PlanetPtr Clone() const;

 private:
    Player owner_;
    int num_ships_;
    int locked_ships_;
    int growth_rate_;
    double effective_growth_rate_;

    std::vector<FleetSummary> incoming_fleets_;
    mutable std::vector<PlanetState> prediction_;
    mutable bool update_prediction_;

    void UpdatePrediction() const;

};

#endif
