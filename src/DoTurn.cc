#include <cmath>
#include <cstdlib>
#include "PlanetWars.h"
#include "DoTurn.h"

void Defence(PlanetWars& pw);

void DoTurn(PlanetWars& pw, int turn) {
    Defence(pw);

  std::vector<PlanetPtr> my_planets = pw.PlanetsOwnedBy(ME);

  // Expansion and attack
  int source = -1;
  int source_num_ships = 0;
  for (int i = 0; i < my_planets.size(); ++i) {
    const PlanetPtr p = my_planets[i];
    source = p->PlanetID();
    source_num_ships = p->Ships();

      // (3) Find the weakest enemy or neutral planet.
      int dest = -1;
      int dest_score = 999999;
      int dest_delay = 0;
        int required_ships = 0;
      std::vector<PlanetPtr> planets = pw.PlanetsOwnedBy(ME); // TODO: Use projected ownership instead
      std::vector<PlanetPtr> notmyplanets = pw.PlanetsNotOwnedBy(ME); // TODO: Use projected ownership instead
      planets.insert(planets.end(), notmyplanets.begin(), notmyplanets.end()); 
      for (int i = 0; i < planets.size(); ++i) {
        const PlanetPtr p = planets[i];
        int growth_rate = Map::GrowthRate(p->PlanetID());
        if ( growth_rate == 0 ) {
            continue;
        }

        // Don't need to do anything if we will own the planet
        int future_owner = p->FutureOwner();
        if ( future_owner == 1) {
            continue;
        }

        // If a neutral planet is closer to an enemy then ignore it
        if ( future_owner == NEUTRAL ) {

            // find the closest enemy
            const std::vector<int>& sorted = Map::PlanetsByDistance( p->PlanetID() );
            int closest_enemy = -1; 
            for (int i=1; i < sorted.size(); ++i) {
                if ( pw.GetPlanet(sorted[i])->Owner() == ENEMY ) {
                    closest_enemy = sorted[i];
                    break;
                }
            }
            int closest_me = -1; 
            for (int i=1; i < sorted.size(); ++i) {
                if ( pw.GetPlanet(sorted[i])->Owner() == ME ) {
                    closest_me = sorted[i];
                    break;
                }
            }

            if ( closest_enemy >= 0 && Map::Distance( closest_enemy, p->PlanetID() ) <= Map::Distance( closest_me, p->PlanetID() ) ) {
                continue;
            }
        }

        // Estimate the number of days required to break even after capturing a planet
        int score = 99999;
        int delay = 0;
        int cost = 0;

        int days = Map::Distance( p->PlanetID(), source );
        int future_days = p->FutureDays();
        if ( days > future_days ) {
            // easy case: we arrive after all the other fleets
            PlanetState prediction = p->FutureState( days );
            cost = prediction.ships;
            if ( future_owner ) {
                // For an enemy planet:
                //   the number of days to travel to the planet 
                //   + time to regain units on planet at start of flight
                //   + time to regain units due to growth rate of enemy
                //   - time to offset enemy units that will no longer be produced
                score = ceil((double)cost/growth_rate/2.0) + days;
            }
            else {
                // For a neutral planet:
                //   the number of days to travel to the planet
                //   + time to regain units spent
                score = ceil((double)cost/growth_rate) + days;
            }
        }
        else {
            // hard case: we can arrive before some other ships
            // we know that this planet is (or will be) eventually be owned 
            // by an enemy
            int best_score = 99999;
            int best_cost = 0;

            // determine the best day to arrive on
            for ( int arrive = future_days; arrive >= days; --arrive ) {
                // TODO: Another magic param 
                cost = p->Cost( arrive ) + 5; 

                score = (int)ceil((double)cost/growth_rate/2.0) + arrive; 
                if ( score < best_score ) {
                    best_score = score;
                    delay = arrive - days;
                    best_cost = cost;
                }
            }
            score = best_score;
            cost = best_cost;
            // cost = p.Cost( days ) + 3; 
            //
            // score = (int)ceil((double)cost/growth_rate/2.0) + days; 
        }

        if (score < dest_score) {
          dest_score = score;
          dest = p->PlanetID();
          required_ships = cost;
          dest_delay = delay;
        }
      }

      if (source >= 0 && dest >= 0 ) {
        // TODO: we should only send units when required_ships > 0 BUT
        //       it does help redirstribute ships.
        // Keep until we have proper redistribution
          // determine the number of ships required to take over the planet
          required_ships += 3;

          required_ships -= dest_delay * Map::GrowthRate(source);

          // ensure that we have enough ships to take over the planet.
          // TODO: Determine best parameter
          if ( required_ships <= 0 || required_ships > (int)(source_num_ships) ) {
            continue;
          }

          if ( dest_delay > 0 ) {
            LOG( "  DELAYED ORDER: " << source << " " << dest << " " << required_ships << " | " << dest_delay << std::endl);
          }
          pw.IssueOrder(Order( source, dest, required_ships ), dest_delay);
        }
    }
}

void Defence(PlanetWars& pw) {
    std::vector<PlanetPtr> my_planets = pw.PlanetsOwnedBy(ME);

    for (int i = 0; i < my_planets.size(); ++i) {
        PlanetPtr p = my_planets[i];
        // if ( p.FutureOwner() != ME ) {
        //     continue;
        // }

        // TODO: IF this is an important planet then we must protect
        // Else we can run away if AFTER all order have been issued we are still 
        // under attack

        int required_ships = p->RequiredShips();

        if ( required_ships > 3 ) { 
            // -3 works slightly better than just 0
            // TODO: Find the best number
            p->RemoveShips(required_ships-3);

            LOG( " " << "Locking " << (required_ships - 3) << " ships on planet " << p->PlanetID() << std::endl );
        }
    }
}
