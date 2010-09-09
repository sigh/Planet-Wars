#include <iostream>
#include <fstream>
#include "model/PlanetWars.h"

std::ofstream log_file("debug.log", std::ios::app);

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
  if ( 0 && pw.MyFleets().size() >= 1 ) {
    Fleet f = pw.MyFleets()[0];
    log_file << f.TotalTripLength() << " "
                   << f.TurnsRemaining() << " "
                   << pw.Distance( f.SourcePlanet(), f.DestinationPlanet() );
  }

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
  double source_score = -999999.0;
  int source_num_ships = 0;
  std::vector<Planet> my_planets = pw.MyPlanets();
  for (int i = 0; i < my_planets.size(); ++i) {
    const Planet& p = my_planets[i];
    source = p.PlanetID();
    source_num_ships = p.NumShips();

      // (3) Find the weakest enemy or neutral planet.
      int dest = -1;
      double dest_score = 999999.0;
      std::vector<Planet> not_my_planets = pw.NotMyPlanets(); // TODO: Use projected ownership instead
      for (int i = 0; i < not_my_planets.size(); ++i) {
        const Planet& p = not_my_planets[i];
        double score;

        // Don't send ships to this planet if we have already started attacking it
        // bool is_attacked = false;
        // for ( int f=0; f < my_fleets.size(); ++f) {
        //     if ( my_fleets[f].DestinationPlanet() == p.PlanetID() ) {
        //         is_attacked = true;
        //         break;
        //     }
        // }
        // if ( is_attacked ) {
        //     continue;
        // }

        // Estimate the number of days required to break even after capturing a planet
        int days = pw.Distance( p.PlanetID(), source );
        PlanetState projection = p.Projection( days );
        if ( projection.owner == 1 ) { 
            continue;
        }
        else if ( projection.owner ) { 
            // For an enemy planet:
            //   the number of days to travel to the planet 
            //   + time to regain units on planet at start of flight
            //   + time to regain units due to growth rate of enemy
            //   - time to offset enemy units that will no longer be produced
            score = (double)projection.ships/p.GrowthRate()/2.0 + days;
        }                                                                                             
        else {
            // For a neutral planet:
            //   the number of days to travel to the planet
            //   + time to regain units spent
            score = (double)projection.ships/p.GrowthRate() + days;
        }
        if (score < dest_score) {
          dest_score = score;
          dest = p.PlanetID();
        }
      }

      if (source >= 0 && dest >= 0) {
          // determine the number of ships required to take over the planet

          const Planet &p_dest = pw.GetPlanet(dest);
            int required_ships = p_dest.Projection(pw.Distance( dest, source )).ships + 3;

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
  log_file.close();
  return 0;
}
