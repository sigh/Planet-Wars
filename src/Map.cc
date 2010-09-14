#include <cmath>
#include <vector>
#include <algorithm>
#include "Map.h"

namespace Map {
    std::vector<int> growth_rates_;
    std::vector< std::pair<double,double> > positions_;
    std::vector< std::vector<int> > distances_;
    std::vector< std::vector<int> > planets_by_distance_;
    int num_planets_;

    int GrowthRate(int planet) {
        return growth_rates_[planet];
    }

    int Distance(int source, int dest) {
        return distances_[source][dest];
    }

    int NumPlanets() {
        return num_planets_;
    }

    void AddPlanet(int growth_rate, double x, double y) {
        growth_rates_.push_back(growth_rate);
        positions_.push_back( std::pair<double,double>(x,y) );
        ++num_planets_;
    }

    std::vector<int> PlanetsByDistance(int planet) {
        return planets_by_distance_[planet];
    }

    // Initialise state
    // ================

    void InitDistances() {
        // Ensure distance matrix can hold all planets
        distances_.resize(num_planets_);

        // calculate distances
        for ( int i=0; i<num_planets_; ++i) {
            distances_[i].resize(num_planets_);

            double i_x = positions_[i].first;
            double i_y = positions_[i].second;

            distances_[i][i] = 0;
            for ( int j=0; j<i; ++j) {
                double dx = i_x - positions_[j].first;
                double dy = i_y - positions_[j].second;
                distances_[i][j] = distances_[j][i] = (int)ceil(sqrt(dx * dx + dy * dy));
            }
        }
    }

    // compare planets by distances to the given source planet
    struct ComparePlanetsByDistance {
        std::vector<int>& d_;
        ComparePlanetsByDistance(int planet) : d_(distances_[planet]) {}
        bool operator()(int a, int b) { return d_[a] < d_[b]; }
    };

    void InitPlanetsByDistance() {
        planets_by_distance_.resize(num_planets_);

        for ( int i=0; i<num_planets_; ++i ) {
            // initialise array with planet ids
            planets_by_distance_[i].resize(num_planets_);
            for ( int j=0; j<num_planets_; ++j) {
                planets_by_distance_[i][j] = j;
            }

            std::sort( 
                planets_by_distance_[i].begin(),
                planets_by_distance_[i].end(),
                ComparePlanetsByDistance(i)
            );
        }
    }

    void Init() {
        InitDistances();
        InitPlanetsByDistance();
    }

}
