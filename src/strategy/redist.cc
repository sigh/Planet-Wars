#include "redist.h"
#include "../Log.h"
#include "../Config.h"

#include <map>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

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
        if ( locked_planets[p_id] || p->Ships(true) <= 0 ) {
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

        state.IssueOrder(Fleet(source_id, dest_id, p->Ships(true)));
        LOG( " Redistributing from " << source_id << " to " << dest_id );
    }
}
