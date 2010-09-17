#include <cmath>
#include <cstdlib>
#include <algorithm>
#include "PlanetWars.h"
#include "DoTurn.h"

void Defence(PlanetWars& pw);
int ClosestPlanetByOwner(const PlanetWars& pw, int planet, int player);

const int INF = 999999;

void DoTurn(PlanetWars& pw, int turn) {
    Defence(pw);

    std::vector<PlanetPtr> planets = pw.Planets();
    std::vector< std::pair<int,int> > scores;

    for (int p_id=0; p_id<planets.size(); ++p_id) {
        const PlanetPtr p = planets[p_id];
        int growth_rate = Map::GrowthRate(p_id);

        // Ignore 0 growth planets for now
        // LATER: put it back in FOR ENEMIES ONLY
        // Beware of divide by 0
        if ( growth_rate == 0 ) {
            continue;
        }

        // Don't need to do anything if we will own the planet
        // TODO: Handle in the Distribution phase
        int future_owner = p->FutureOwner();
        if ( future_owner == 1) {
            continue;
        }

        // If a neutral planet is closer to an enemy then ignore it
        if ( future_owner == NEUTRAL ) {

            int closest_enemy = ClosestPlanetByOwner( pw, p_id, ENEMY );
            int closest_me = ClosestPlanetByOwner( pw, p_id, ME );

            if ( closest_enemy >= 0 && Map::Distance( closest_enemy, p_id ) <= Map::Distance( closest_me, p_id) ) {
                continue;
            }
        }

        // sort MY planets in order of distance the target planet
        const std::vector<int>& all_sorted = Map::PlanetsByDistance( p_id );
        std::vector<int> my_sorted;
        for ( int i=1; i<all_sorted.size(); ++i) {
            if ( all_sorted[i] != p_id && pw.GetPlanet(all_sorted[i])->Owner() == ME ) {
                my_sorted.push_back( all_sorted[i] );
            }
        }

        // Determine best case 
        int future_days = p->FutureDays();
        int available_ships = 0;
        int cost = INF;
        int score = INF;
        int delay = 0;

        for ( int i=0; i<my_sorted.size() && cost > available_ships; ++i) {
            int source = my_sorted[i];
            const PlanetPtr source_p = pw.GetPlanet(source);
            
            available_ships += source_p->Ships();

            int distance = Map::Distance( p_id, source );
            delay = 0;
            cost = INF;

            if ( distance > future_days ) {
                // easy case: we arrive after all the other fleets
                PlanetState prediction = p->FutureState( distance );
                cost = prediction.ships;
                if ( future_owner ) {
                    // For an enemy planet:
                    //   the number of days to travel to the planet 
                    //   + time to regain units on planet at start of flight
                    //   + time to regain units due to growth rate of enemy
                    //   - time to offset enemy units that will no longer be produced
                    score = ceil((double)cost/growth_rate/2.0) + distance;
                }
                else {
                    // For a neutral planet:
                    //   the number of days to travel to the planet
                    //   + time to regain units spent
                    score = ceil((double)cost/growth_rate) + distance;
                }
            }
            else {
                // hard case: we can arrive before some other ships
                // we know that this planet is (or will be) eventually be owned 
                // by an enemy
                int best_score = 99999;
                int best_cost = 0;

                // determine the best day to arrive on
                for ( int arrive = future_days; arrive >= distance; --arrive ) {
                    // TODO: Another magic param 
                    int cost = p->Cost( arrive ) + 5; 

                    int score = (int)ceil((double)cost/growth_rate/2.0) + arrive; 
                    if ( score < best_score ) {
                        best_score = score;
                        delay = arrive - distance;
                        best_cost = cost;
                    }
                }
                score = best_score;
                cost = best_cost;
            }
            cost += 3;
        }

        // if ( cost <= available_ships ) {  // Try putting this back in
            scores.push_back( std::pair<int,int>(score, p_id) );
        // }
    }


    // sort scores
    std::sort(scores.begin(), scores.end());

    // start attacking planets based on score
    for ( int i=0; i < scores.size(); ++i) {
        std::pair<int,int> s = scores[i];
        int p_id = s.second;
        PlanetPtr p = pw.GetPlanet(p_id);

        // list of the orders that we will want to execute
        std::vector<Order> orders;


        // Do the cost anaysis again
        // TODO: refactor so that the code isn't dupluicated

        // sort MY planets in order of distance the target planet
        const std::vector<int>& all_sorted = Map::PlanetsByDistance( p_id );
        std::vector<int> my_sorted;
        for ( int i=1; i<all_sorted.size(); ++i) {
            if ( all_sorted[i] != p_id && pw.GetPlanet(all_sorted[i])->Owner() == ME ) {
                my_sorted.push_back( all_sorted[i] );
            }
        }


        // Determine best case 
        int future_days = p->FutureDays();
        int future_owner = p->FutureOwner();
        int growth_rate = Map::GrowthRate(p_id);
        int available_ships = 0;
        int cost = INF;
        int score = INF;
        int delay = 0;

        for ( int i=0; i<my_sorted.size() && cost > available_ships; ++i) {
            int source = my_sorted[i];
            const PlanetPtr source_p = pw.GetPlanet(source);
            
            available_ships += source_p->Ships();

            int distance = Map::Distance( p_id, source );
            delay = 0;

            if ( distance > future_days ) {
                // easy case: we arrive after all the other fleets
                PlanetState prediction = p->FutureState( distance );
                cost = prediction.ships;
                if ( future_owner ) {
                    // For an enemy planet:
                    //   the number of days to travel to the planet 
                    //   + time to regain units on planet at start of flight
                    //   + time to regain units due to growth rate of enemy
                    //   - time to offset enemy units that will no longer be produced
                    score = ceil((double)cost/growth_rate/2.0) + distance;
                }
                else {
                    // For a neutral planet:
                    //   the number of days to travel to the planet
                    //   + time to regain units spent
                    score = ceil((double)cost/growth_rate) + distance;
                }
            }
            else {
                // hard case: we can arrive before some other ships
                // we know that this planet is (or will be) eventually be owned 
                // by an enemy
                int best_score = 99999;
                int best_cost = 0;

                // determine the best day to arrive on
                for ( int arrive = future_days; arrive >= distance; --arrive ) {
                    // TODO: Another magic param 
                    int cost = p->Cost( arrive ) + 5; 

                    int score = (int)ceil((double)cost/growth_rate/2.0) + arrive; 
                    if ( score < best_score ) {
                        best_score = score;
                        delay = arrive - distance;
                        best_cost = cost;
                    }
                }
                score = best_score;
                cost = best_cost;
            }
            cost += 3;

            if ( cost > available_ships ) {
                orders.push_back( Order(source, p_id, source_p->Ships(), delay) ); 
            }
            else {
                orders.push_back( Order(source, p_id, cost - ( available_ships - source_p->Ships() ), delay) ); 
            }
        }

        if ( cost <= available_ships ) {  // Try putting this back in
            scores.push_back( std::pair<int,int>(score, p_id) );
        }
        else {
            break;
        }

        // If we reached here we want to actually execute the orders
        for ( int j=0; j<orders.size(); j++ ) {
            pw.IssueOrder(orders[j]);
        }

        // TODO: remove this
        break;
    }


   //   if (source >= 0 && dest >= 0 ) {
   //     // TODO: we should only send units when required_ships > 0 BUT
   //     //       it does help redirstribute ships.
   //     // Keep until we have proper redistribution
   //       // determine the number of ships required to take over the planet
   //       required_ships += 3;

   //       required_ships -= dest_delay * Map::GrowthRate(source);

   //       // ensure that we have enough ships to take over the planet.
   //       // TODO: Determine best parameter
   //       if ( required_ships <= 0 || required_ships > (int)(source_num_ships) ) {
   //         continue;
   //       }

   //       if ( dest_delay > 0 ) {
   //         LOG( "  DELAYED ORDER: " << source << " " << dest << " " << required_ships << " | " << dest_delay << std::endl);
   //       }
   //       pw.IssueOrder(Order( source, dest, required_ships ), dest_delay);
   //     }
   // }
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

// Determine the clostest planet to the given planet owned by player
// Return -1 if no planet found
int ClosestPlanetByOwner(const PlanetWars& pw, int planet, int player) {
    const std::vector<int>& sorted = Map::PlanetsByDistance( planet );

    int closest = -1;

    for (int i=1; i < sorted.size(); ++i) {
        if ( pw.GetPlanet(sorted[i])->Owner() == player ) {
            closest = sorted[i];
            break;
        }
    }

    return closest;
}
