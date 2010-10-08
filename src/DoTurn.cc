#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <map>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include "GameState.h"
#include "Log.h"
#include "Config.h"
#include "DoTurn.h"

#include "strategy/attack.h"

void Defence(GameState& state);
DefenceExclusions AntiRage(GameState& state);
int AntiRageRequiredShips(const GameState &state, const PlanetPtr& my_planet, const PlanetPtr& enemy_planet);

void Redistribution(GameState& state);

void DoTurn(const GameState& initial_state, std::vector<Fleet>& orders) {
    int my_planet_count = initial_state.PlanetsOwnedBy(ME).size();
    if ( my_planet_count == 0 ) {
        LOG("We have no planets, we can make no actions");
        return;
    }

    // Create a mutable version of the state
    GameState state = initial_state;

    Defence(state);

    DefenceExclusions defence_exclusions = AntiRage(state);

    Attack(state, defence_exclusions);

    Redistribution(state);

    LOG("Finishing up");

    // Populate orders array with the orders that were generated
    orders = state.Orders();
}

/*
 * Defence
 */

// Lock the required number of ships onto planets that are under attack
void Defence(GameState& state) {
    static bool defence = Config::Value<bool>("defence"); 
    if ( ! defence ) return;

    LOG("Defence phase");

    std::vector<PlanetPtr> my_planets = state.PlanetsOwnedBy(ME);
    foreach ( PlanetPtr& p, my_planets ) {
        // TODO: IF this is an important planet then we must protect
        // Else we can run away if AFTER all order have been issued we are still 
        // under attack
        
        int required_ships = p->RequiredShips();

        if ( required_ships > 0 ) { 
            // TODO: This might be impacting the prediction so see if we can fix it
            //       (Idea send a zero day fleet)
            p->LockShips(required_ships);

            LOG( " Locking " << required_ships << " ships on planet " << p->id );
        }
    }
}

DefenceExclusions AntiRage(GameState& state) {
    DefenceExclusions defence_exclusions; 

    // Anti rge level
    // 0: None    |
    // 1: closest | Increasing number of ships locked
    // 2: max     |
    // 3: sum     v
    static int anti_rage_level = Config::Value<int>("antirage");
    static bool have_defence_exclusions = Config::Value<bool>("antirage.exlusions");
    if ( anti_rage_level == 0 ) return defence_exclusions;

    LOG("Antirage phase");

    std::map<int,bool> frontier_planets = state.FrontierPlanets(ME);
    std::pair<int,bool> item;
    foreach ( item, frontier_planets ) {
        if ( ! item.second ) continue;

        PlanetPtr p = state.Planet(item.first);

        int ships_locked = 0;

        // the planet we are protecting ourselves from
        PlanetPtr rage_planet;

        if ( anti_rage_level == 1 ) {
            // defend against only the closest enemy
            PlanetPtr closest_enemy = state.ClosestPlanetByOwner(p, ENEMY);
            if ( closest_enemy ) {
                ships_locked = AntiRageRequiredShips(state, p, closest_enemy );
                rage_planet = closest_enemy;
            }
        }
        else {
            // defend against all enemies
            int max_required_ships = 0;
            int sum_required_ships = 0;
            std::vector<PlanetPtr> enemy_planets = state.PlanetsOwnedBy(ENEMY);
            foreach ( PlanetPtr& enemy_planet, enemy_planets ) {
                int required_ships = AntiRageRequiredShips(state, p, enemy_planet );
                if ( required_ships > max_required_ships ) {
                    max_required_ships = required_ships;
                    rage_planet = enemy_planet;
                }
                sum_required_ships += required_ships;
            }

            ships_locked = anti_rage_level == 2 ? max_required_ships : sum_required_ships;
        }

        if ( ships_locked > 0 ) {
            ships_locked = p->LockShips(ships_locked);
            if ( anti_rage_level != 3 && have_defence_exclusions ) {
                defence_exclusions[p->id] = std::pair<int,int>(rage_planet->id, ships_locked);
            }
            LOG( " Locking " << ships_locked << " ships on planet " << p->id );
        }
    }

    return defence_exclusions;
}

int AntiRageRequiredShips(const GameState &state, const PlanetPtr& my_planet, const PlanetPtr& enemy_planet) {
    int distance = Map::Distance(enemy_planet->id, my_planet->id);
    int required_ships = enemy_planet->Ships() - distance*my_planet->GrowthRate();
    if ( required_ships <= 0 ) return 0;

    // enslist help
    const std::vector<int>& sorted = Map::PlanetsByDistance( my_planet->id );
    foreach ( int i, sorted ) {
        const PlanetPtr p = state.Planet(i);
        if ( p->Owner() != ME ) continue;

        int help_distance =  Map::Distance(my_planet->id, i);
        if ( help_distance >= distance ) break;
        required_ships -= p->Ships() + p->ShipExcess(distance-help_distance-1);
    }

    if ( required_ships <= 0 ) return 0;

    return required_ships;
}

// We do this because compile_anything script for ai-contest does
// not recurse into sub directories
#include "strategy/attack.cpp"

/*
 * Redistribution
 */

void Redistribution(GameState& state) {
    static bool redist = Config::Value<bool>("redist");
    static bool use_future = Config::Value<bool>("redist.future");
    if ( !redist ) return;

    LOG("Redistribution phase");

    std::map<int,bool> locked_planets = use_future ? state.FutureFrontierPlanets(ME) : state.FrontierPlanets(ME);

    // determine distances of all planets to closest ENEMY
    const std::vector<PlanetPtr> planets = state.Planets();
    std::map<int,int> distances;
    foreach ( PlanetPtr p, planets ) {
        PlanetPtr enemy = state.ClosestPlanetByOwner(p, ENEMY);
        distances[p->id] = enemy ? Map::Distance(p->id, enemy->id) : 0;
    }

    std::map<int,int> redist_map;

    // determine 1 step redistribution
    foreach ( PlanetPtr p, planets ) {
        int p_id = p->id;

        // We only want to redistribute from planets where we are the future owner :D
        if ( p->FutureOwner() != ME ) {
            continue;
        }

        // Filter out planets we don't want to redistribute from
        if ( locked_planets[p_id] || p->Ships() <= 0 ) {
            continue;
        }

        // Find the closest planet with a lower distance to the enemy
        int distance = distances[p_id];
        const std::vector<int>& sorted = Map::PlanetsByDistance( p_id );
        int closest = -1;
        foreach ( int s_id, sorted ) {
            PlanetPtr s = state.Planet(s_id);
            int s_owner = use_future ? s->FutureOwner() : s->Owner();
            if ( s_owner == ME && distances[s_id] < distance ) {
                closest = s_id;

                // If we comment this out then we just throw all units at the planet
                // closest to the enemy. This leaves our units quite vunerable
                // But for some reason it is sometimes effective. Find out when and WHY!
                break; // Redist has this commented out
            }
        }

        if (closest >= 0) {
            redist_map[p_id] = closest;
        }
    }

    std::pair<int,int> item;
    std::map<int,int>::iterator found;

    std::map<int,int> original_map(redist_map);

    // send ships directly to the end of the redistribution chain
    // this means that more ships get to the front lines quicker
    // it also means that planets in the middle have permanently locked ships
    //    (because growth and arrivals are processed AFTER we make our
    //    move order but BEFORE the orders are carried out)
    //
    // Possibly experiment with more conserative skips
    //    (maybe for defence mode - higher growth rate, lower ships)
    foreach (item, redist_map ) { 
        int source = item.first;
        int dest = item.second;
        int current = source;
        while ( (found = redist_map.find(dest)) != redist_map.end() ) {
            current = dest;
            dest = found->second;
        }
        redist_map[source] = dest;
    }

    static int redist_slack = Config::Value<int>("redist.slack");
    // route through a staging planet if there is enough slack
    foreach ( item, redist_map ) {
        int source = item.first;
        int dest = item.second;
        int current = source;
        int distance = Map::Distance(source, dest);

        while ( (found = original_map.find(current)) != redist_map.end() && found->second != dest ) {
            current = found->second;
            if ( Map::Distance(source, current) + Map::Distance(current,dest) <= redist_slack + distance ) {
                LOG(" hopping through " << current);
                redist_map[source] = current;
                break;
            }
        }
        redist_map[source] = dest;
    }

    // Output the redistributions
    foreach (item, redist_map ) { 
        int source_id = item.first;
        int dest_id = item.second;
        PlanetPtr p = state.Planet(source_id);

        // Can't redisribute from unowned planets!
        if ( p->Owner() != ME ) continue;

        // Don't mess up neutral stealing!
        // This prevents us prematurely sending ships to a planet which we might be neutral stealing from the enemy
        if ( state.Planet( dest_id )->FutureState( Map::Distance( source_id, dest_id ) ).owner == NEUTRAL ) continue;

        state.IssueOrder(Fleet(source_id, dest_id, p->Ships()));
        LOG( " Redistributing from " << source_id << " to " << dest_id );
    }
}
/*
 

void Flee(PlanetWars& pw);
void Harass(PlanetWars& pw, int planet, std::vector<Fleet>& orders);

bool SortByGrowthRate(PlanetPtr a, PlanetPtr b) {
    return Map::GrowthRate(a->PlanetID()) < Map::GrowthRate(b->PlanetID()); 
}

void Flee(PlanetWars& pw) {
    static bool flee = Config::Value<bool>("flee");
    if ( ! flee  ) return;

    LOG("Flee phase");

    std::vector<PlanetPtr> my_planets = pw.PlanetsOwnedBy(ME);
    sort(my_planets.begin(), my_planets.end(), SortByGrowthRate);

    std::vector<PlanetPtr> planets = pw.Planets();

    for ( int i=0; i<my_planets.size(); ++i ) {
        PlanetPtr p = my_planets[i]; 
        if ( p->FutureOwner() == ME ) continue;

        int p_id = p->PlanetID();
        int ships = p->FutureState(0).ships;

        int closest_distance = INF;
        int destination = -1;

        // if the future owner is not me then see if we can use our ships elsewhere
        for ( int j=0; j<planets.size(); ++j ) {
            if ( j == p_id ) continue; 

            PlanetPtr dest = planets[j];
            if ( dest->Owner() == NEUTRAL ) continue;
            if ( dest->FutureOwner() != ENEMY ) continue;

            int distance = Map::Distance(p_id, j);
            int cost = dest->Cost(distance);

            LOG( "   Cost from " << p_id << " to " << j << ": " << cost );

            if ( cost > ships ) continue;

            if ( distance < closest_distance ) {
                closest_distance = distance;
                destination = j;
            }
        }

        if ( destination >= 0 ) {
            pw.IssueOrder(Fleet(p_id, destination, ships));
            LOG( " Fleeing from " << p_id << " to " << destination );
        }
    }
}





void Harass(PlanetWars& pw, int planet, std::vector<Fleet>& orders) {
    if ( orders.size() < 1 ) return;
    Fleet& order = orders[0];

    // ensure that the destination is owned by the enemy
    if ( pw.GetPlanet(order.dest)->Owner() != ENEMY ) return;

    // ensure that the source is a frontier planet
    std::map<int,bool> frontier_planets = FrontierPlanets(pw, ME);
    if ( ! frontier_planets[order.source] ) return;

    // Issue the harassment order
    LOG( " Harassing " << order.dest << " from " << order.source );
    pw.IssueOrder(order);
}
*/
