#ifndef PLAYER_H_
#define PLAYER_H_

#include <ostream>

enum Player {
    ME = 1,
    NEUTRAL = 0,
    ENEMY = -1
};   

// Negative switches to the opposite player
inline Player operator-(Player player) {
    return static_cast<Player>(-(int)player);
}

#endif // PLAYER_H_
