#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <map>
#include "PlanetWars.h"
#include "Log.h"
#include "DoTurn.h"

void Defence(PlanetWars& pw);
void Redistribution(PlanetWars& pw);
int ClosestPlanetByOwner(const PlanetWars& pw, int planet, int player);
std::pair<int,int> CostAnalysis(const PlanetWars& pw, PlanetPtr p);
std::pair<int,int> CostAnalysis(const PlanetWars& pw, PlanetPtr p, std::vector<Order>& orders);

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
        if ( future_owner == ME) {
            continue;
        }

        // If a neutral planet is closer to an enemy then ignore it
        if ( future_owner == NEUTRAL ) {

            int closest_enemy = ClosestPlanetByOwner( pw, p_id, ENEMY );
            int closest_me = ClosestPlanetByOwner( pw, p_id, ME );

            if ( closest_enemy >= 0 && closest_me >= 0  && Map::Distance( closest_enemy, p_id ) <= Map::Distance( closest_me, p_id) ) {
                continue;
            }
        }

        int score = CostAnalysis(pw, p).second;

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

        int cost = CostAnalysis(pw, p, orders).first;

        if ( cost >= INF ) {
            // This case happens when cost > available_ships

            // If we don't have enough ships to capture the next best planet then WAIT
            //  This serves a few purposes:
            //  1. Prevent us overextending our forces
            //  2. If we wait we might get the required resources later
            //  3. This helps against Rage tactics
            break;
        }
        
        // update delays so all fleet arrive at once
        // TODO: Try leaving this out (Might harm neutral attack/overtakes)
        const Order& last_order = orders.back();
        int last_arrival = last_order.delay + Map::Distance( last_order.source, last_order.dest );
        for ( int j=0; j<orders.size()-1; ++j ) {
            Order& order = orders[j];
            order.delay = last_arrival - Map::Distance( order.source, order.dest );
        }


        // If we reached here we want to actually execute the orders
        for ( int j=0; j<orders.size(); ++j ) {
            pw.IssueOrder(orders[j]);
        }
    }

    Redistribution(pw);
}

// Lock the required number of ships onto planets that are under attack
void Defence(PlanetWars& pw) {
    std::vector<PlanetPtr> my_planets = pw.PlanetsOwnedBy(ME);

    for (int i = 0; i < my_planets.size(); ++i) {
        PlanetPtr p = my_planets[i];

        // TODO: IF this is an important planet then we must protect
        // Else we can run away if AFTER all order have been issued we are still 
        // under attack
        
        int required_ships = p->RequiredShips();

        if ( required_ships > 0 ) { 
            // TODO: This might be impacting the prediction so see if we can fix it
            //       (Idea send a zero day fleet)
            p->RemoveShips(required_ships);

            LOG( " " << "Locking " << required_ships << " ships on planet " << p->PlanetID() );
        }
    }
}

void Redistribution(PlanetWars& pw) {
    const std::vector<PlanetPtr> enemy_planets = pw.PlanetsOwnedBy(ENEMY);

    // Lock planets which are closest to an enemy
    std::map<int,bool> locked_planets;
    for (int i = 0; i < enemy_planets.size(); ++i) {
        int p = enemy_planets[i]->PlanetID();
        locked_planets[ClosestPlanetByOwner(pw,p,ME)] = true;
    }

    // determine distances of own planets
    const std::vector<PlanetPtr> my_planets = pw.PlanetsOwnedBy(ME);
    std::map<int,int> distances;
    for (int i = 0; i < my_planets.size(); ++i) {
        int me = my_planets[i]->PlanetID();
        int enemy = ClosestPlanetByOwner(pw, me, ENEMY);
        distances[me] = enemy >= 0 ? Map::Distance(me, enemy) : 0;
    }

    for (int i = 0; i < my_planets.size(); ++i) {
        PlanetPtr p = my_planets[i];
        int p_id = p->PlanetID();

        // Filter out planets we don't want to redistribute from
        if ( locked_planets[p_id] || p->Ships() <= 0 ) {
            continue;
        }

        // Find the closest planet with a lower distance to the enemy
        int distance = distances[p_id];
        const std::vector<int>& sorted = Map::PlanetsByDistance( p_id );
        int closest = -1;
        for ( int j=0; j<sorted.size(); ++j ) {
            int s_id = sorted[j];
            if ( pw.GetPlanet(s_id)->Owner() == ME && distances[s_id] < distance ) {
                closest = s_id;
                distance = distances[s_id];
            }
        }

        // If we found a planet, then send half available ships there
        if (closest >= 0) {
            LOG( " Redistributing from " << p_id << " to " << closest );
            pw.IssueOrder(Order(p_id, closest, p->Ships()));
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

std::pair<int,int> CostAnalysis(const PlanetWars& pw, PlanetPtr p) {
    std::vector<Order> orders;
    return CostAnalysis(pw, p, orders);
}

// Does a cost analysis for taking over planet p and populates orders with the orders required
// to do so.
// Returns (cost, score)
std::pair<int,int> CostAnalysis(const PlanetWars& pw, PlanetPtr p, std::vector<Order>& orders) {
    int p_id = p->PlanetID();

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
        if ( cost < 0 ) {
            cost = 0;
        }
        // cost -= delay * Map::GrowthRate(source);

        int required_ships = 0;
        if ( cost > available_ships ) {
            required_ships = source_p->Ships();
        }
        else {
            required_ships = cost - ( available_ships - source_p->Ships() );
        }

        if ( required_ships < 0 ) {
            // Fix the WTF
            LOG_ERROR( "WTF: " << cost << " " << available_ships << " " << source_p->Ships() );
        }

        orders.push_back( Order(source, p_id, required_ships, delay) ); 
    }

    if ( cost > available_ships ) {
        cost = INF;
    }

    return std::pair<int,int>(cost,score);
}
