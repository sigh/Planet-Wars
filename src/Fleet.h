#ifndef FLEET_H_
#define FLEET_H_

#include "Map.h"

struct Order {
    public:
        int source;
        int dest;
        int ships;
        int delay;
        Order(int source, int dest, int ships, int delay=0)
            : source(source), dest(dest), ships(ships), delay(delay) {
                // TODO: Warn and log error about empty order
       }
};

struct Fleet {
    public:
        int owner;
        int dest;
        int ships;
        int remaining;
        Fleet() {};
        Fleet(int owner, const Order &o) : owner(owner) {
            dest = o.dest;
            ships = o.ships;
            remaining = Map::Distance( o.source, o.dest );
        }
};

// my ships, enemy ships
typedef std::pair<int, int> FleetSummary;

#endif
