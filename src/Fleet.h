#ifndef FLEET_H_
#define FLEET_H_

#include "Map.h"

struct Fleet {
    public:
        int owner;      // player id of owner
        int source;     // planet id of source
        int dest;       // planet id of destination
        int ships;      // number of ships in fleet
        int launch;     // number of days until launch (negative if fleet is already in flight)

        Fleet(int owner, int source, int dest, int ships, int launch=0)
            : owner(owner), source(source), dest(dest), ships(ships), launch(launch) {
                // TODO: Warn and log error about empty order
        }
    
        // creating a fleet for issuing an order
        Fleet(int source, int dest, int ships, int launch=0)
            : owner(ME), source(source), dest(dest), ships(ships), launch(launch) {
                // TODO: Warn and log error about empty order
        }

        // number of days until fleet arrives
        inline int remaining() const { 
            return Map::Distance(source, dest) + launch;
        }

        // number of days a fleet is in flight
        inline int length() const { 
            return Map::Distance(source, dest);
        }
};

// my ships, enemy ships
class FleetSummary {
    public:
        inline int& operator [](int player) {
           return player == ME ? me_ : enemy_; 
        }

        inline int operator [](int player) const {
           return player == ME ? me_ : enemy_; 
        }

        // difference in ships (favoring player)
        inline int delta(int player=1) const {
            if ( player == ME ) {
                return me_ - enemy_;
            }
            else {
                return enemy_ - me_;
            }
        }

        // return true if there are no ships in the fleet summary
        inline bool empty() const {
            return enemy_ == 0 && me_ == 0;
        }

    private:
        int enemy_;
        int me_;
};

#endif
