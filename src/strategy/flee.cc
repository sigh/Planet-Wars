#include "attack.h"

#include "../Log.h"
#include "../Config.h"

#include <cmath>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

bool SortByGrowthRate(PlanetPtr a, PlanetPtr b) {
    return a->GrowthRate() < b->GrowthRate();
}

void Flee(GameState &state, Player player) {
    static bool flee = Config::Value<bool>("flee");
    if ( ! flee  ) return;

    LOG("Flee phase");

    std::vector<PlanetPtr> my_planets = state.PlanetsOwnedBy(player);
    sort(my_planets.begin(), my_planets.end(), SortByGrowthRate);

    std::vector<PlanetPtr> planets = state.Planets();

    foreach ( PlanetPtr &p, my_planets ) {
        if ( p->FutureOwner() == player ) continue;

        int p_id = p->id;
        int ships = p->FutureState(0).ships;

        int closest_distance = INF;
        int destination = -1;

        // if the future owner is not me then see if we can use our ships elsewhere
        foreach ( PlanetPtr &dest, planets ) {
            int dest_id = dest->id;
            if ( dest_id == p_id ) continue; 
            if ( p->GrowthRate() >= dest->GrowthRate() ) break;

            if ( dest->Owner() == NEUTRAL ) continue;
            if ( dest->FutureOwner() != -player ) continue;

            int distance = Map::Distance(p_id, dest_id);
            int cost = dest->Cost(distance, player);

            LOG( "   Cost from " << p_id << " to " << dest_id << ": " << cost );

            if ( cost > ships ) continue;

            if ( distance < closest_distance ) {
                closest_distance = distance;
                destination = dest_id;
            }
        }

        if ( destination >= 0 ) {
            state.IssueOrder(Fleet(player, p_id, destination, ships));
            LOG( " Fleeing from " << p_id << " to " << destination );
        }
    }
}

