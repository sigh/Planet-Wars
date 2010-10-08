#ifndef ATTACK_H_
#define ATTACK_H_

#include "../GameState.h"

// TODO: put this in defence.h
typedef std::map<int, std::pair<int,int> > DefenceExclusions;

void Attack(GameState& state, DefenceExclusions& defence_exclusions);

#endif
