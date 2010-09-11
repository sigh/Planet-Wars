#include <cmath>
#include "Map.h"

namespace Map {
    std::vector<int> growth_rates_;
    std::vector< std::pair<double,double> > positions_;

    int GrowthRate(int planet) {
        return growth_rates_[planet];
    }

    int Distance(int source, int dest) {
        double dx = positions_[source].first  - positions_[dest].first;
        double dy = positions_[source].second - positions_[dest].second;
        return (int)ceil(sqrt(dx * dx + dy * dy));
    }

    int NumPlanets() {
        return growth_rates_.size();
    }

    void AddPlanet(int growth_rate, double x, double y) {
        growth_rates_.push_back(growth_rate);
        positions_.push_back( std::pair<double,double>(x,y) );
    }
}
