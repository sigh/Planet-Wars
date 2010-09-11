#include <iostream>
#include <fstream>
#include <cmath>
#include "PlanetWars.h"

std::pair<int, int> BreakEven() {
    int ships_to_send;
    int break_even;


    return std::pair<int,int>(ships_to_send, break_even);
}

// The DoTurn function is where your code goes. The PlanetWars object contains
// the state of the game, including information about all planets and fleets
// that currently exist. Inside this function, you issue orders using the
// pw.IssueOrder() function. For example, to send 10 ships from planet 3 to
// planet 8, you would say pw.IssueOrder(3, 8, 10).
//
// There is already a basic strategy in place here. You can use it as a
// starting point, or you can throw it out entirely and replace it with your
// own. Check out the tutorials and articles on the contest website at
// http://www.ai-contest.com/resources.
void DoTurn(PlanetWars& pw, int turn_number) {

  std::vector<Fleet> my_fleets = pw.MyFleets();

  // defence
  std::vector<Fleet> enemy_fleets = pw.EnemyFleets();
  for ( int i=0; i < enemy_fleets.size(); ++i ) {
    Planet& dest = pw.GetPlanet(enemy_fleets[i].DestinationPlanet());
    if ( dest.Owner() == 1 ) {
        dest.RemoveShips( enemy_fleets[i].NumShips() );
    }
  }

  // (2) Find my strongest planet.
  int source = -1;
  int source_num_ships = 0;
  std::vector<Planet> my_planets = pw.MyPlanets();
  for (int i = 0; i < my_planets.size(); ++i) {
    const Planet& p = my_planets[i];
    source = p.PlanetID();
    source_num_ships = p.NumShips();

      // (3) Find the weakest enemy or neutral planet.
      int dest = -1;
      int dest_score = 999999;
      int dest_delay = 0;
        int required_ships = 0;
      std::vector<Planet> planets = pw.Planets(); // TODO: Use projected ownership instead
      for (int i = 0; i < planets.size(); ++i) {
        const Planet& p = planets[i];
        if ( p.GrowthRate() == 0 ) {
            continue;
        }

        // Don't need to do anything if we will own the planet
        int future_owner = p.FutureOwner();
        if ( future_owner == 1) {
            continue;
        }

        // Estimate the number of days required to break even after capturing a planet
        int score = 99999;
        int delay = 0;
        int cost = 0;

        int days = pw.Distance( p.PlanetID(), source );
        int future_days = p.FutureDays();
        if ( days > future_days ) {
            // easy case: we arrive after all the other fleets
            PlanetState prediction = p.FutureState( days );
            cost = prediction.ships;
            if ( future_owner ) {
                // For an enemy planet:
                //   the number of days to travel to the planet 
                //   + time to regain units on planet at start of flight
                //   + time to regain units due to growth rate of enemy
                //   - time to offset enemy units that will no longer be produced
                score = ceil((double)cost/p.GrowthRate()/2.0) + days;
            }
            else {
                // For a neutral planet:
                //   the number of days to travel to the planet
                //   + time to regain units spent
                score = ceil((double)cost/p.GrowthRate()) + days;
            }
        }
        else {
            // hard case: we can arrive before some other ships
            // we know that this planet is (or will be) eventually be owned 
            // by an enemy
            cost = p.Cost(days);
            score = ceil((double)cost/p.GrowthRate()/2.0) + days; 
            // determine the best day to arrive on
            // for ( int arrive = days; arrive <= future_days; ++arrive ) {
            //     int result = p.Cost( arrive ); 
            // }
        }

        if (score < dest_score) {
          dest_score = score;
          dest = p.PlanetID();
          required_ships = cost;
        }
      }

      if (source >= 0 && dest >= 0 ) {
        // TODO: we should only send units when required_ships > 0 BUT
        //       it does help redirstribute ships.
        // Keep until we have proper redistribution
      
          // determine the number of ships required to take over the planet
          required_ships += 3;

          // ensure that we have enough ships to take over the planet.
          if ( required_ships > (int)(source_num_ships * 0.75) ) {
            continue;
          }

          // (4) Send half the ships from my strongest planet to the weakest
          // planet that I do not own.
            int num_ships = required_ships;
            pw.IssueOrder(source, dest, num_ships);
          }
    }
}

// This is just the main game loop that takes care of communicating with the
// game engine for you. You don't have to understand or change the code below.
int main(int argc, char *argv[]) {
  std::string current_line;
  std::string map_data;
  int turn_number = 0;
  while (true) {
    int c = std::cin.get();
    current_line += (char)c;
    if (c == '\n') {
      if (current_line.length() >= 2 && current_line.substr(0, 2) == "go") {
          ++turn_number;
        PlanetWars pw(map_data);
        map_data = "";
        DoTurn(pw, turn_number);
	pw.FinishTurn();
      } else {
        map_data += current_line;
      }
      current_line = "";
    }
  }
  return 0;
}
