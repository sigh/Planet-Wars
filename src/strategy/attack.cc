#include "attack.h"

#include "../Log.h"
#include "../Config.h"

#include <cmath>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

int AttackPlanet(const GameState& state, PlanetPtr p, const DefenceExclusions& defence_exclusions, Player player);
int AttackPlanet(const GameState& state, PlanetPtr p, const DefenceExclusions& defence_exclusions, std::vector<Fleet>& orders, Player player);
int ScoreEdge(const GameState& state, PlanetPtr dest, PlanetPtr source, int available_ships, int source_ships, int& cost, std::vector<Fleet>& orders, Player player);
std::vector<PlanetPtr> FindTargets(const GameState& state, const DefenceExclusions& defence_exclusions, Player player);
void GreedyAttack(GameState& state, const DefenceExclusions& defence_exclusions, std::vector<PlanetPtr>& targets, Player player);
int CombinationAttack(GameState& state, const DefenceExclusions& defence_exclusions, std::vector<PlanetPtr>& targets, Player player, unsigned int=0);
int EvalState( const GameState& state );

void Attack(GameState& state, DefenceExclusions& defence_exclusions, Player player) {
    static bool attack = Config::Value<bool>("attack");
    if ( ! attack  ) return;

    LOG("Attack phase");

    std::vector<PlanetPtr> targets = FindTargets(state, defence_exclusions, player);

    CombinationAttack(state, defence_exclusions, targets, player);
}

int AttackPlanet(const GameState& state, PlanetPtr p, const DefenceExclusions& defence_exclusions, Player player) {
    std::vector<Fleet> orders;
    return AttackPlanet(state, p, defence_exclusions, orders, player);
}

// Does a cost analysis for taking over planet p and populates orders with the orders required
// to do so.
// Returns score
int AttackPlanet(const GameState& state, PlanetPtr p, const DefenceExclusions& defence_exclusions, std::vector<Fleet>& orders, Player player) {
    int p_id = p->id;

    // sort player planets in order of distance the target planet
    const std::vector<int>& all_sorted = Map::PlanetsByDistance( p_id );
    std::vector<int> my_sorted;
    foreach ( int source_id, all_sorted ) {
        if ( state.Planet(source_id)->Owner() == player ) {
            my_sorted.push_back( source_id );
        }
    }

    // determine distance to closest enemy
    PlanetPtr closest_enemy = state.ClosestPlanetByOwner( p, -player );
    int closest_enemy_distance = closest_enemy ? Map::Distance(closest_enemy->id, p_id) : INF;
    int future_owner = p->FutureOwner();

    int score = INF;
    int cost = INF;

    int available_ships = 0;
    foreach ( int source_id, my_sorted ) {
        if ( cost <= available_ships ) break;

        const PlanetPtr source = state.Planet(source_id);
        int source_ships = source->Ships(true);

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
        }
        
        available_ships += source_ships;

        score = ScoreEdge(state, p, source, available_ships, source_ships, cost, orders, player);
    }

    // If the cost is too large then we can generate no orders
    if ( cost > available_ships ) {
        orders.clear();
    }

    if ( orders.size() > 0 ) {
        LOG( "  score of planet " << p_id << " = " << score << " (" <<  cost << ") after " << orders.back().launch << " days" );
    }
    else {
        LOG( "  not enough ships to attack " << p_id );
    }

    return score;
}

// Score one source -> dest fleet
int ScoreEdge(const GameState& state, PlanetPtr dest, PlanetPtr source, int available_ships, int source_ships, int& cost, std::vector<Fleet>& orders, Player player) {
    static double distance_scale = Config::Value<double>("cost.distance_scale");
    static double growth_scale   = Config::Value<double>("cost.growth_scale");
    static int    cost_offset    = Config::Value<int>("cost.offset");

    int source_id = source->id;
    int dest_id = dest->id;

    int future_days = dest->FutureDays();
    int future_owner = dest->FutureOwner();
    int growth_rate = dest->GrowthRate();

    int distance = Map::Distance( dest_id, source_id );
    cost = INF;
    int score = INF;
    int extra_delay = 0;

    int ships_within_range = state.ShipsWithinRange(dest, distance, -player);
    if ( distance > future_days ) {
        // easy case: we arrive after all the other fleets
        PlanetState prediction = dest->FutureState( distance );
        cost = prediction.ships;


        if ( future_owner ) {

            int score_cost = cost + ships_within_range;
            if ( dest->Owner() == -player ) cost = score_cost;

            // TODO: determine the best factor for distance
            score = (int)((double)score_cost/growth_rate/growth_scale + distance*distance_scale);
            // score = distance + distance/2;
            LOG( "  to attack " << dest_id << " from " << source_id << ": cost = " << cost << ", score_cost = " << score_cost << ", score = " << score );
        }
        else {
            // For a neutral planet:
            //   the number of days to travel to the planet
            //   + time to regain units spent
            int score_cost = cost + ships_within_range;
            score = ceil((double)score_cost/growth_rate) + distance;
        }
    }
    else {
        // hard case: we can arrive before some other ships
        // we know that this planet is (or will be) eventually be owned 
        // by an enemy
        int best_score = INF;
        int best_cost = 0;

        // determine the best day to arrive on, we search up to 1 day AFTER the last fleet arrives
        for ( int arrive = future_days+1; arrive >= distance; --arrive ) {
            // TODO: Another magic param 
            int cost = dest->Cost( arrive, player ); 
            int score_cost = cost + ships_within_range;
            if ( dest->Owner() == -player ) cost = score_cost;

            // int score = arrive + arrive/2;
            int score = (int)((double)score_cost/growth_rate/growth_scale + arrive*distance_scale);
            if ( score < best_score ) {
                best_score = score;
                extra_delay = arrive - distance;
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

    orders.push_back( Fleet(player, source_id, dest_id, required_ships, extra_delay) ); 

    return score;
}

// Find potential targets in order of score
std::vector<PlanetPtr> FindTargets(const GameState& state, const DefenceExclusions& defence_exclusions, Player player) {
    LOG(" Finding targets");

    std::vector<PlanetPtr> planets = state.Planets();
    std::vector< std::pair<int,int> > scores;

    std::map<int,bool> targets_1 = state.FrontierPlanets(-player); 
    std::map<int,bool> targets_2 = state.FutureFrontierPlanets(-player); 
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
        int future_owner = p->FutureOwner();
        if ( future_owner == player) {
            continue;
        }

        // If a neutral planet is closer to an enemy then ignore it
        if ( future_owner == NEUTRAL ) {

            PlanetPtr closest_enemy = state.ClosestPlanetByOwner( p, -player );
            PlanetPtr closest_me = state.ClosestPlanetByOwner( p, player );

            if ( closest_enemy && closest_me && Map::Distance( closest_enemy->id, p_id ) <= Map::Distance( closest_me->id, p_id) ) {
                continue;
            }
        }

        // If the planet is owned by an enemy and it is NOT a frontier planet then ignore
        if ( p->Owner() == -player && ! ( targets_1[p_id] || targets_2[p_id] ) ) {
            continue;
        }

        // Don't try to neutral steal planets that we would not otherwise attack
        if ( future_owner == -player && p->Owner() == NEUTRAL && ! targets_2[p_id] ) {
            continue;
        }

        int score = AttackPlanet(state, p, defence_exclusions, player);
        scores.push_back( std::pair<int,int>(score, p_id) );
    }

    // sort scores
    std::sort(scores.begin(), scores.end());

    // create the result
    std::vector<PlanetPtr> result;
    std::pair<int,int> s;
    foreach ( s, scores ) {
        result.push_back(state.Planet(s.second));
    }

    return result;
}

void GreedyAttack(GameState& state, const DefenceExclusions& defence_exclusions, std::vector<PlanetPtr>& targets, Player player) {
    LOG(" Starting attacks (greedy)");

    // start attacking planets based on score
    foreach ( PlanetPtr p, targets ) {
        // list of the orders that we will want to execute
        std::vector<Fleet> orders;

        AttackPlanet(state, p, defence_exclusions, orders, player);

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

int CombinationAttack(GameState& state, const DefenceExclusions& defence_exclusions, std::vector<PlanetPtr>& targets, Player player, unsigned int i) {
    if ( i == 0 ) {
        LOG(" Starting attacks (combination)");
    }

    if ( i >= targets.size() ) {
        return EvalState(state);
    }

    GameState attack_state = state;

    int attack_eval = -INF;

    // Try attacking targets[i]
    std::vector<Fleet> orders;
    AttackPlanet(attack_state, targets[i], defence_exclusions, orders, player);
    if ( ! orders.empty() ) {
        const Fleet& last_order = orders.back();

        // update delays so that all fleets arrive at a neutral at the same time
        if ( attack_state.Planet(last_order.dest)->FutureOwner() == NEUTRAL ) {
            int last_arrival = last_order.launch + Map::Distance( last_order.source, last_order.dest );
            foreach ( Fleet& order, orders ) {
                order.launch = last_arrival - Map::Distance( order.source, order.dest );
            }
        }

        // If we reached here we want to actually execute the orders
        foreach ( Fleet& order, orders ) {
            attack_state.IssueOrder(order);
        }

        attack_eval = CombinationAttack(attack_state, defence_exclusions, targets, player, i+1);
    }

    // Try not attacking targets[i]
    int eval = CombinationAttack(state, defence_exclusions, targets, player, i+1);

    LOG( "  " << i << ": attack_state " << attack_eval << ", state " << eval );
    // update with most valuable state
    if ( attack_eval > eval ) {
        state = attack_state;
        eval = attack_eval;
    }

    return eval;
}

int EvalState( const GameState& state ) { 
    int score = 0;
    foreach ( const PlanetPtr& p, state.Planets() ) {
        PlanetState s = p->FutureState(50);
        score += s.owner * s.ships;
    }
    return score;
}
