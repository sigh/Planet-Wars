#ifndef REDIST_H_
#define REDIST_H_

#include "../GameState.h"

typedef std::map<int, std::pair<int,int> > DefenceExclusions;

void Redistribution(GameState& state);

#endif


