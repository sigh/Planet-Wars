#include "defence.h"
#include "../Log.h"
#include "../Config.h"

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

int AntiRageRequiredShips(const GameState &state, const PlanetPtr& my_planet, const PlanetPtr& enemy_planet);

// Lock the required number of ships onto planets that are under attack
void Defence(GameState& state, Player player) {
    static bool defence = Config::Value<bool>("defence"); 
    if ( ! defence ) return;

    LOG("Defence phase");

    std::vector<PlanetPtr> my_planets = state.PlanetsOwnedBy(player);
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

DefenceExclusions AntiRage(GameState& state, Player player) {
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

    std::map<int,bool> frontier_planets = state.FrontierPlanets(player);
    std::pair<int,bool> item;
    foreach ( item, frontier_planets ) {
        if ( ! item.second ) continue;

        PlanetPtr p = state.Planet(item.first);

        int ships_locked = 0;

        // the planet we are protecting ourselves from
        PlanetPtr rage_planet;

        if ( anti_rage_level == 1 ) {
            // defend against only the closest enemy
            PlanetPtr closest_enemy = state.ClosestPlanetByOwner(p, -player);
            if ( closest_enemy ) {
                ships_locked = AntiRageRequiredShips(state, p, closest_enemy );
                rage_planet = closest_enemy;
            }
        }
        else {
            // defend against all enemies
            int max_required_ships = 0;
            int sum_required_ships = 0;
            std::vector<PlanetPtr> enemy_planets = state.PlanetsOwnedBy(-player);
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
            if (  have_defence_exclusions ) {
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

    Player player = my_planet->Owner();

    // enslist help
    const std::vector<int>& sorted = Map::PlanetsByDistance( my_planet->id );
    foreach ( int i, sorted ) {
        const PlanetPtr p = state.Planet(i);
        if ( p->Owner() != player ) continue;

        int help_distance =  Map::Distance(my_planet->id, i);
        if ( help_distance >= distance ) break;
        required_ships -= p->Ships(true) + p->ShipExcess(distance-help_distance-1);
    }

    if ( required_ships <= 0 ) return 0;

    return required_ships;
}
