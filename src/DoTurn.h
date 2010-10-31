#ifndef DO_TURN_H_
#define DO_TURN_H_

#include "Player.h"

void DoTurn(const GameState& state, std::vector<Fleet>& orders, Player player = ME);

#endif
