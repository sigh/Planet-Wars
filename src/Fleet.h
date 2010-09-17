#ifndef FLEET_H_
#define FLEET_H_

#include "Map.h"

struct Order {
    public:
        int source;
        int dest;
        int ships;
        Order(int source, int dest, int ships)
            : source(source), dest(dest), ships(ships) {
                // TODO: Warn and log error about empty order
            if  (ships < 0) {
                ships = 0;
            }
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
