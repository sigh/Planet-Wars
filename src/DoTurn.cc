#include <cstdlib>

#include "GameState.h"
#include "Log.h"
#include "Config.h"
#include "DoTurn.h"

#include "strategy/attack.h"
#include "strategy/defence.h"
#include "strategy/redist.h"
#include "strategy/flee.h"

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
