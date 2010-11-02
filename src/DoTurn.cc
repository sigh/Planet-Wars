#include <cstdlib>

#include "GameState.h"
#include "Log.h"
#include "Config.h"
#include "DoTurn.h"

#include "strategy/attack.h"
#include "strategy/defence.h"
#include "strategy/redist.h"

void Flee(GameState &state, Player player);

void DoTurn(const GameState& initial_state, std::vector<Fleet>& orders, Player player /* = ME */) {
    int my_planet_count = initial_state.PlanetsOwnedBy(player).size();
    if ( my_planet_count == 0 ) {
        LOG("We have no planets, we can make no actions");
        return;
    }

    // Create a mutable version of the state
    GameState state = initial_state;

    Defence(state, player);

    DefenceExclusions defence_exclusions = AntiRage(state, player);

    Attack(state, defence_exclusions, player);

    Flee(state, player);

    Redistribution(state);

    LOG("Finishing up");

    // Populate orders array with the orders that were generated
    orders = state.Orders();
}

// We do this because compile_anything script for ai-contest does
// not recurse into sub directories

#include "strategy/defence.cc"
#include "strategy/attack.cc"
#include "strategy/redist.cc"

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

/*
 

void Flee(PlanetWars& pw);
void Harass(PlanetWars& pw, int planet, std::vector<Fleet>& orders);







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
