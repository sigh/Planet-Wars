#ifndef DEFENCE_H_
#define DEFENCE_H_

#include "../GameState.h"

typedef std::map<int, std::pair<int,int> > DefenceExclusions;

void Defence(GameState& state, Player player);
DefenceExclusions AntiRage(GameState& state, Player player);

#endif

