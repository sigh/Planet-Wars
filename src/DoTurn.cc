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

typedef std::map<int, std::pair<int,int> > DefenceExclusions;

void Defence(GameState& state);
DefenceExclusions AntiRage(GameState& state);
int AntiRageRequiredShips(const GameState &state, const PlanetPtr& my_planet, const PlanetPtr& enemy_planet);
void Attack(GameState& state, DefenceExclusions& defence_exclusions);

int ScorePlanet(const GameState& state, PlanetPtr p, const DefenceExclusions& defence_exclusions);
int ScorePlanet(const GameState& state, PlanetPtr p, const DefenceExclusions& defence_exclusions, std::vector<Fleet>& orders);

void DoTurn(const GameState& initial_state, std::vector<Fleet>& orders) {
    // Create a mutable version of the state
    GameState state = initial_state;

    Defence(state);

    DefenceExclusions defence_exclusions = AntiRage(state);

    Attack(state, defence_exclusions);

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

/*
 * Attack
 */

void Attack(GameState& state, DefenceExclusions& defence_exclusions) {
    static bool attack = Config::Value<bool>("attack");
    if ( ! attack  ) return;

    LOG("Attack phase");

    std::vector<PlanetPtr> planets = state.Planets();
    std::vector< std::pair<int,int> > scores;

    std::map<int,bool> targets_1 = state.FrontierPlanets(ENEMY); 
    std::map<int,bool> targets_2 = state.FutureFrontierPlanets(ENEMY); 
    for (unsigned int p_id=0; p_id<planets.size(); ++p_id) {
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

            PlanetPtr closest_enemy = state.ClosestPlanetByOwner( p, ENEMY );
            PlanetPtr closest_me = state.ClosestPlanetByOwner( p, ME );

            if ( closest_enemy && closest_me && Map::Distance( closest_enemy->id, p_id ) <= Map::Distance( closest_me->id, p_id) ) {
                continue;
            }
        }

        // If the planet is owned by an enemy and it is NOT a frontier planet then ignore
        if ( p->Owner() == ENEMY && ! ( targets_1[p_id] || targets_2[p_id] ) ) {
            continue;
        }

        // Don't try to neutral steal planets that we would not otherwise attack
        if ( future_owner == ENEMY && p->Owner() == NEUTRAL && ! targets_2[p_id] ) {
            continue;
        }

        int score = ScorePlanet(state, p, defence_exclusions);
        scores.push_back( std::pair<int,int>(score, p_id) );
    }

    // sort scores
    std::sort(scores.begin(), scores.end());

    LOG(" Starting attacks");

    // start attacking planets based on score
    std::pair<int,int> s;
    foreach ( s, scores ) {
        PlanetPtr p = state.Planet(s.second);

        // list of the orders that we will want to execute
        std::vector<Fleet> orders;

        ScorePlanet(state, p, defence_exclusions, orders);

        if ( orders.empty() ) {
            // This case happens when cost > available_ships

            // If we don't have enough ships to capture the next best planet then WAIT
            //  This serves a few purposes:
            //  1. Prevent us overextending our forces
            //  2. If we wait we might get the required resources later
            //  3. This helps against Rage tactics
            // Harass(state, p->id, orders);
            break;
        }

        // update delays so all fleet arrive at once
        // TODO: Try leaving this out (Might harm neutral attack/overtakes)
        const Fleet& last_order = orders.back();
        int last_arrival = last_order.launch + Map::Distance( last_order.source, last_order.dest );
        foreach ( Fleet& order, orders ) {
            order.launch = last_arrival - Map::Distance( order.source, order.dest );
        }

        // If we reached here we want to actually execute the orders
        foreach ( Fleet& order, orders ) {
            state.IssueOrder(order);
        }
        
    }
}

int ScorePlanet(const GameState& state, PlanetPtr p, const DefenceExclusions& defence_exclusions) {
    std::vector<Fleet> orders;
    return ScorePlanet(state, p, defence_exclusions, orders);
}

// Does a cost analysis for taking over planet p and populates orders with the orders required
// to do so.
// Returns score
int ScorePlanet(const GameState& state, PlanetPtr p, const DefenceExclusions& defence_exclusions, std::vector<Fleet>& orders) {
    return 0;
}
/*
 

void Flee(PlanetWars& pw);
void Redistribution(PlanetWars& pw);
void Harass(PlanetWars& pw, int planet, std::vector<Fleet>& orders);

int ScoreEdge(const PlanetWars& pw, PlanetPtr dest, PlanetPtr source, int available_ships, int source_ships, int delay, int& cost, std::vector<Fleet>& orders);

void DoTurn(PlanetWars& pw, int turn) {
    int my_planet_count = pw.PlanetsOwnedBy(ME).size();
    if ( my_planet_count == 0 ) {
        LOG("We have no planets, we can make no actions");
        return;
    }

    Defence(pw);

    DefenceExclusions defence_exclusions = AntiRage(pw);

    Attack(pw, defence_exclusions);

    Redistribution(pw);

    Flee(pw);

    LOG("Finishing up");
}



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




void Redistribution(PlanetWars& pw) {
    static bool redist = Config::Value<bool>("redist");
    static bool use_future = Config::Value<bool>("redist.future");
    if ( !redist ) return;

    LOG("Redistribution phase");

    std::map<int,bool> locked_planets = use_future ? FutureFrontierPlanets(pw, ME) : FrontierPlanets(pw,ME);

    // determine distances of all planets to closest ENEMY
    const std::vector<PlanetPtr> planets = pw.Planets();
    std::map<int,int> distances;
    foreach ( PlanetPtr p, planets ) {
        int planet = p->PlanetID();
        int enemy = ClosestPlanetByOwner(pw, planet, ENEMY);
        distances[planet] = enemy >= 0 ? Map::Distance(planet, enemy) : 0;
    }

    std::map<int,int> redist_map;

    // determine 1 step redistribution
    foreach ( PlanetPtr p, planets ) {
        int p_id = p->PlanetID();

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
        for ( int j=0; j<sorted.size(); ++j ) {
            int s_id = sorted[j];
            PlanetPtr s = pw.GetPlanet(s_id);
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
        PlanetPtr p = pw.GetPlanet(source_id);

        // Can't redisribute from unowned planets!
        if ( p->Owner() != ME ) continue;

        // Don't mess up neutral stealing!
        // This prevents us prematurely sending ships to a planet which we might be neutral stealing from the enemy
        if ( pw.GetPlanet( dest_id )->FutureState( Map::Distance( source_id, dest_id ) ).owner == NEUTRAL ) continue;

        pw.IssueOrder(Fleet(source_id, dest_id, p->Ships()));
        LOG( " Redistributing from " << source_id << " to " << dest_id );
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

int ScorePlanet(const PlanetWars& pw, PlanetPtr p, const DefenceExclusions& defence_exclusions) {
    std::vector<Fleet> orders;
    return ScorePlanet(pw, p, defence_exclusions, orders);
}

// Does a cost analysis for taking over planet p and populates orders with the orders required
// to do so.
// Returns score
int ScorePlanet(const PlanetWars& pw, PlanetPtr p, const DefenceExclusions& defence_exclusions, std::vector<Fleet>& orders) {
    static int max_delay = Config::Value<int>("attack.max_delay");
    int p_id = p->PlanetID();

    // sort MY planets in order of distance the target planet
    const std::vector<int>& all_sorted = Map::PlanetsByDistance( p_id );
    std::vector<int> my_sorted;
    for ( int i=1; i<all_sorted.size(); ++i) {
        if ( all_sorted[i] != p_id && pw.GetPlanet(all_sorted[i])->Owner() == ME ) {
            my_sorted.push_back( all_sorted[i] );
        }
    }

    // determine distance to closest enemy
    int closest_enemy_distance = INF;
    int closest_enemy = ClosestPlanetByOwner( pw, p_id, ENEMY );
    if ( closest_enemy >= 0 ) {
        closest_enemy_distance = Map::Distance(closest_enemy, p_id);
    }
    int future_owner = p->FutureOwner();

    // Determine best case 
    int best_score = INF;
    int best_cost = INF;
    std::vector<Fleet> best_orders;

    for ( int delay=0; delay <= max_delay; ++delay ) {
        int score = INF;
        int cost = INF;
        std::vector<Fleet> current_orders;

        int available_ships = 0;
        for ( int i=0; i<my_sorted.size() && cost > available_ships; ++i) {
            const PlanetPtr source = pw.GetPlanet(my_sorted[i]);

            int source_id = source->PlanetID();
            int source_ships = source->Ships();

            if ( future_owner == NEUTRAL ) {
                // Don't attack neutral planets closer to the enemy
                int distance = Map::Distance( source_id, p_id );
                // if ( closest_enemy_distance <= distance && p->FutureDays() < distance ) {
                if ( closest_enemy_distance <= distance ) {
                    // We do not want this move to be considered AT ALL
                    // So give it the highest score
                    cost = INF;
                    score = INF;
                    break;
                }
            }

            // determine if we have any ship locking exclusion for this 
            //   source/dest pair
            DefenceExclusions::const_iterator d_it = defence_exclusions.find(source_id);
            if ( d_it != defence_exclusions.end() && d_it->second.first == p_id ) {
                LOG( " Lifting defence exclusion for " << d_it->second.second << " ships on planet " << source_id );
                source_ships += d_it->second.second;
                available_ships += delay*Map::GrowthRate(source_id);
            }
            
            available_ships += source_ships;

            score = ScoreEdge(pw, p, source, available_ships, source_ships, delay, cost, current_orders);
        }

        // If the cost is too large then we can generate no orders
        if ( cost > available_ships ) {
            current_orders.clear();
        }

        if ( score < best_score ) {
            best_score = score;
            best_cost = cost;
            best_orders = current_orders;
        }
    }

    orders.insert(orders.end(), best_orders.begin(), best_orders.end());

    if ( orders.size() > 0 ) {
        LOG( "  score of planet " << p_id << " = " << best_score << " (" <<  best_cost << ") after " << orders.back().launch << " days" );
    }

    return best_score;
}

// Score one source -> dest fleet
int ScoreEdge(const PlanetWars& pw, PlanetPtr dest, PlanetPtr source, int available_ships, int source_ships, int delay, int& cost, std::vector<Fleet>& orders) {
    static double distance_scale = Config::Value<double>("cost.distance_scale");
    static double growth_scale   = Config::Value<double>("cost.growth_scale");
    static double delay_scale    = Config::Value<double>("cost.delay_scale");
    static int    cost_offset    = Config::Value<int>("cost.offset");
    static bool   use_egr        = Config::Value<bool>("cost.use_egr");

    int source_id = source->PlanetID();
    int dest_id = dest->PlanetID();

    int future_days = dest->FutureDays();
    int future_owner = dest->FutureOwner();
    int growth_rate = Map::GrowthRate(dest_id);

    int distance = Map::Distance( dest_id, source_id );
    cost = INF;
    int score = INF;
    int extra_delay = 0;

    if ( delay + distance > future_days ) {
        // easy case: we arrive after all the other fleets
        PlanetState prediction = dest->FutureState( distance + delay );
        cost = prediction.ships;

        if ( future_owner ) {
            if ( use_egr ) {
                PlanetState future_state = dest->FutureState(future_days);
                cost = future_state.ships + ( distance - future_days ) * dest->EffectiveGrowthRate();
            }

            int score_cost = cost + ShipsWithinRange(pw,dest,distance,ENEMY); 

            // TODO: determine the best factor for distance
            score = (int)((double)score_cost/growth_rate/growth_scale + delay/delay_scale + distance*distance_scale);
            // score = distance + distance/2;
        }
        else {
            // For a neutral planet:
            //   the number of days to travel to the planet
            //   + time to regain units spent
            int score_cost = cost + ShipsWithinRange(pw,dest,distance,ENEMY); 
            score = ceil((double)score_cost/growth_rate) + delay/delay_scale + distance;
        }
    }
    else {
        // hard case: we can arrive before some other ships
        // we know that this planet is (or will be) eventually be owned 
        // by an enemy
        int best_score = INF;
        int best_cost = 0;

        // determine the best day to arrive on, we search up to 12 day AFTER the last fleet arrives
        for ( int arrive = future_days+1; arrive >= distance + delay; --arrive ) {
            // TODO: Another magic param 
            int cost = dest->Cost( arrive ); 
            int score_cost = cost + ShipsWithinRange(pw,dest,distance, ENEMY); 

            // int score = arrive + arrive/2;
            int score = (int)((double)score_cost/growth_rate/growth_scale + delay/delay_scale + (arrive-delay)*distance_scale);
            if ( score < best_score ) {
                best_score = score;
                extra_delay = arrive - distance - delay;
                best_cost = cost;
            }
        }

        score = best_score;
        cost = best_cost;
    }

    cost += cost_offset;
    if ( cost < 0 ) {
        cost = 0;
    }

    int required_ships = 0;
    if ( cost > available_ships ) {
        required_ships = source_ships;
    }
    else {
        required_ships = source_ships - ( available_ships - cost );
    }

    if ( required_ships < 0 ) {
        // Fix the WTF
        LOG_ERROR( "WTF: attacking " << dest_id << ": " << cost << " " << available_ships << " " << source_ships );
    }

    orders.push_back( Fleet(ME, source_id, dest_id, required_ships, delay + extra_delay) ); 

    return score;
}
*/
