#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <map>
#include "PlanetWars.h"
#include "Log.h"
#include "DoTurn.h"

typedef std::map<int, std::pair<int,int> > DefenceExclusions;

void Defence(PlanetWars& pw);
void Flee(PlanetWars& pw);
void Attack(PlanetWars& pw, DefenceExclusions& defence_exclusions);
DefenceExclusions AntiRage(PlanetWars& pw);
int AntiRageRequiredShips(PlanetWars &pw, int my_planet, int enemy_planet);
void Redistribution(PlanetWars& pw);
void Harass(PlanetWars& pw, int planet, std::vector<Order>& orders);
int ClosestPlanetByOwner(const PlanetWars& pw, int planet, int player);
std::pair<int,int> CostAnalysis(const PlanetWars& pw, PlanetPtr p, const DefenceExclusions& defence_exclusions);
std::pair<int,int> CostAnalysis(const PlanetWars& pw, PlanetPtr p, const DefenceExclusions& defence_exclusions, std::vector<Order>& orders);
std::map<int,bool> FrontierPlanets(const PlanetWars& pw, int player);
std::map<int,bool> FutureFrontierPlanets(const PlanetWars& pw, int player);

int ScoreEdge(PlanetPtr dest, PlanetPtr source, int available_ships, int source_ships, int delay, int& cost, std::vector<Order>& orders);

const int INF = 999999;

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

void Attack(PlanetWars& pw, DefenceExclusions& defence_exclusions) {
    static bool attack = Config::Value<bool>("attack");
    if ( ! attack  ) return;

    LOG("Attack phase");

    std::vector<PlanetPtr> planets = pw.Planets();
    std::vector< std::pair<int,int> > scores;

    std::map<int,bool> targets_1 = FrontierPlanets(pw, ENEMY); 
    std::map<int,bool> targets_2 = FutureFrontierPlanets(pw, ENEMY); 
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

        // If the planet is owned by an enemy and it is NOT a frontier planet then ignore
        if ( p->Owner() == ENEMY && ! ( targets_1[p_id] || targets_2[p_id] ) ) {
            continue;
        }

        // Don't try to neutral steal planets that we would not otherwise attack
        if ( future_owner == ENEMY && p->Owner() == NEUTRAL && ! targets_2[p_id] ) {
            continue;
        }

        int score = CostAnalysis(pw,p, defence_exclusions).second;

        scores.push_back( std::pair<int,int>(score, p_id) );
    }

    // sort scores
    std::sort(scores.begin(), scores.end());

    LOG(" Starting attacks");

    // start attacking planets based on score
    for ( int i=0; i < scores.size(); ++i) {
        std::pair<int,int> s = scores[i];
        int p_id = s.second;
        PlanetPtr p = pw.GetPlanet(p_id);

        // list of the orders that we will want to execute
        std::vector<Order> orders;

        int cost = CostAnalysis(pw, p, defence_exclusions, orders).first;

        if ( cost >= INF ) {
            // This case happens when cost > available_ships

            // If we don't have enough ships to capture the next best planet then WAIT
            //  This serves a few purposes:
            //  1. Prevent us overextending our forces
            //  2. If we wait we might get the required resources later
            //  3. This helps against Rage tactics
            // Harass(pw, p_id, orders);
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
            pw.IssueOrder(Order(p_id, destination, ships));
            LOG( " Fleeing from " << p_id << " to " << destination );
        }
    }
}

DefenceExclusions AntiRage(PlanetWars& pw) {
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

    std::map<int,bool> frontier_planets = FrontierPlanets(pw, ME);
    std::map<int,bool>::iterator it;

    for ( it=frontier_planets.begin(); it != frontier_planets.end(); ++it ) {
        if ( ! it->second ) continue;

        int p_id = it->first;
        PlanetPtr p = pw.GetPlanet(p_id);

        int ships_locked;

        // the planet we are protecting ourselves from
        int rage_planet = -1;

        if ( anti_rage_level == 1 ) {
            // defend against only the closest enemy
            int closest_enemy = ClosestPlanetByOwner(pw, p_id, ENEMY);
            if ( closest_enemy >= 0 ) {
                ships_locked = AntiRageRequiredShips(pw, p_id, closest_enemy );
                rage_planet = closest_enemy;
            }
        }
        else {
            // defend against all enemies
            int max_required_ships = 0;
            int sum_required_ships = 0;
            std::vector<PlanetPtr> enemy_planets = pw.PlanetsOwnedBy(ENEMY);
            for ( int i=0; i<enemy_planets.size(); ++i ) {
                int required_ships = AntiRageRequiredShips(pw, p_id, enemy_planets[i]->PlanetID() );
                if ( required_ships > max_required_ships ) {
                    max_required_ships = required_ships;
                    rage_planet = enemy_planets[i]->PlanetID();
                }
                sum_required_ships += required_ships;
            }

            ships_locked = anti_rage_level == 2 ? max_required_ships : sum_required_ships;
        }

        if ( ships_locked > 0 ) {
            ships_locked = p->LockShips(ships_locked);
            if ( anti_rage_level != 3 && have_defence_exclusions ) {
                defence_exclusions[p_id] = std::pair<int,int>(rage_planet, ships_locked);
            }
            LOG( " Locking " << ships_locked << " ships on planet " << p->PlanetID() );
        }
    }

    return defence_exclusions;
}

// Lock the required number of ships onto planets that are under attack
void Defence(PlanetWars& pw) {
    static bool defence = Config::Value<bool>("defence"); 
    if ( ! defence ) return;

    LOG("Defence phase");

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
            p->LockShips(required_ships);

            LOG( " Locking " << required_ships << " ships on planet " << p->PlanetID() );
        }
    }
}

int AntiRageRequiredShips(PlanetWars &pw, int my_planet, int enemy_planet) {
    int distance = Map::Distance(enemy_planet, my_planet);
    int required_ships = pw.GetPlanet(enemy_planet)->Ships() - distance*Map::GrowthRate(my_planet);
    if ( required_ships <= 0 ) return 0;

    // enslist help
    const std::vector<int>& sorted = Map::PlanetsByDistance( my_planet );
    for ( int i=1; i<sorted.size(); ++i ) {
        const PlanetPtr p = pw.GetPlanet(sorted[i]);
        if ( p->Owner() != ME ) continue;

        int help_distance =  Map::Distance(my_planet, sorted[i]);
        if ( help_distance >= distance ) break;
        required_ships -= p->Ships() + p->ShipExcess(distance-help_distance-1);
    }

    if ( required_ships <= 0 ) return 0;

    return required_ships;
}

// Find planets closest to the opponent
std::map<int,bool> FrontierPlanets(const PlanetWars& pw, int player) {
    std::map<int,bool> frontier_planets;
    const std::vector<PlanetPtr> opponent_planets = pw.PlanetsOwnedBy(-player);
    for (int i = 0; i < opponent_planets.size(); ++i) {
        int p = opponent_planets[i]->PlanetID();
        int closest = ClosestPlanetByOwner(pw,p,player);
        if ( closest >= 0 ) {
            frontier_planets[closest] = true;
        }
    }
    return frontier_planets;
}

// Find planets that will be closest to the opponent
std::map<int,bool> FutureFrontierPlanets(const PlanetWars& pw, int player) {
    std::map<int,bool> frontier_planets;

    const std::vector<PlanetPtr> opponent_planets = pw.PlanetsOwnedBy(-player);

    // determine future player planets
    for ( int i=0; i<opponent_planets.size(); ++i ) {
        const std::vector<int>& sorted = Map::PlanetsByDistance( opponent_planets[i]->PlanetID() );
        int closest = -1;
        for (int i=1; i < sorted.size(); ++i) {
            if ( pw.GetPlanet(sorted[i])->FutureOwner() == player ) {
                closest = sorted[i];
                break;
            }
        }
        if ( closest >= 0 ) {
            frontier_planets[closest] = true;
        }
    }

    return frontier_planets;
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
    for (int i = 0; i < planets.size(); ++i) {
        int planet = planets[i]->PlanetID();
        int enemy = ClosestPlanetByOwner(pw, planet, ENEMY);
        distances[planet] = enemy >= 0 ? Map::Distance(planet, enemy) : 0;
    }

    std::map<int,int> redist_map;

    // determine 1 step redistribution
    for (int i = 0; i < planets.size(); ++i) {
        PlanetPtr p = planets[i];
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
            int s_owner = use_future ? pw.GetPlanet(s_id)->FutureOwner() : pw.GetPlanet(s_id)->Owner();
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

    std::map<int,int>::iterator it;
    std::map<int,int>::iterator found;

    // send ships directly to the end of the redistribution chain
    // this means that more ships get to the front lines quicker
    // it also means that planets in the middle have permanently locked ships
    //    (because growth and arrivals are processed AFTER we make our
    //    move order but BEFORE the orders are carried out)
    //
    // Possibly experiment with more conserative skips
    //    (maybe for defence mode - higher growth rate, lower ships)
    for ( it=redist_map.begin(); it != redist_map.end(); ++it ) { 
        int source = it->first;
        int dest = it->second;
        int current = source;
        while ( (found = redist_map.find(dest)) != redist_map.end() ) {
            current = dest;
            dest = found->second;
        }
        redist_map[source] = dest;
    }

    // Output the redistributions
    for ( it=redist_map.begin(); it != redist_map.end(); ++it ) { 
        PlanetPtr p = pw.GetPlanet(it->first);

        // Can't redisribute from unowned planets!
        if ( p->Owner() != ME ) continue;

        pw.IssueOrder(Order(it->first, it->second, p->Ships()));
        LOG( " Redistributing from " << it->first << " to " << it->second );
    }
}

void Harass(PlanetWars& pw, int planet, std::vector<Order>& orders) {
    if ( orders.size() < 1 ) return;
    Order& order = orders[0];

    // ensure that the destination is owned by the enemy
    if ( pw.GetPlanet(order.dest)->Owner() != ENEMY ) return;

    // ensure that the source is a frontier planet
    std::map<int,bool> frontier_planets = FrontierPlanets(pw, ME);
    if ( ! frontier_planets[order.source] ) return;

    // Issue the harassment order
    LOG( " Harassing " << order.dest << " from " << order.source );
    pw.IssueOrder(order);
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

std::pair<int,int> CostAnalysis(const PlanetWars& pw, PlanetPtr p, const DefenceExclusions& defence_exclusions) {
    std::vector<Order> orders;
    return CostAnalysis(pw, p, defence_exclusions, orders);
}

// Does a cost analysis for taking over planet p and populates orders with the orders required
// to do so.
// Returns (cost, score)
std::pair<int,int> CostAnalysis(const PlanetWars& pw, PlanetPtr p, const DefenceExclusions& defence_exclusions, std::vector<Order>& orders) {
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
    int available_ships = 0;
    int cost = INF;
    int score = INF;

    for ( int i=0; i<my_sorted.size() && cost > available_ships; ++i) {
        const PlanetPtr source = pw.GetPlanet(my_sorted[i]);

        int source_id = source->PlanetID();
        int source_ships = source->Ships();

        if ( future_owner == NEUTRAL ) {
            // Don't attack neutral planets closer to the enemy
            if ( closest_enemy_distance < Map::Distance( source_id, p_id ) ) {
                cost = INF;
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

        score = ScoreEdge(p, source, available_ships, source_ships, 0, cost, orders);
    }


    if ( orders.size() > 0 ) {
        LOG( "  score of planet " << p_id << " = " << score << " (" <<  cost << ") after " << orders.back().delay << " days" );
        if ( cost > available_ships ) {
            cost = INF;
        }
    }

    return std::pair<int,int>(cost,score);
}

// Score one source -> dest fleet
int ScoreEdge(PlanetPtr dest, PlanetPtr source, int available_ships, int source_ships, int delay, int& cost, std::vector<Order>& orders) {
    static double distance_scale = Config::Value<double>("cost.distance_scale");
    static double growth_scale   = Config::Value<double>("cost.growth_scale");
    static int    cost_offset    = Config::Value<int>("cost.offset");

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
        PlanetState prediction = dest->FutureState( distance );
        cost = prediction.ships;

        if ( future_owner ) {
            // Uncomment to use effective growth rate
            // PlanetState future_state = p->FutureState(future_days);
            // cost = future_state.ships + ( distance - future_days ) * p->EffectiveGrowthRate(future_owner);

            // TODO: determine the best factor for distance
            score = (int)((double)cost/growth_rate/growth_scale + delay + distance*distance_scale);
            // score = distance + distance/2;
        }
        else {
            // For a neutral planet:
            //   the number of days to travel to the planet
            //   + time to regain units spent
            score = ceil((double)cost/growth_rate) + distance + delay;
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

            // int score = arrive + arrive/2;
            int score = (int)((double)cost/growth_rate/growth_scale + arrive*distance_scale);
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

    if ( required_ships <= 0 ) {
        // Fix the WTF
        LOG_ERROR( "WTF: " << cost << " " << available_ships << " " << source->Ships() );
    }

    orders.push_back( Order(source_id, dest_id, required_ships, extra_delay) ); 

    return score;
}
