#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include "PlanetWars.h"

std::ofstream LOG_FILE;

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
void DoTurn(PlanetWars& pw) {

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
        int growth_rate = Map::GrowthRate(p.PlanetID());
        if ( growth_rate == 0 ) {
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

        int days = Map::Distance( p.PlanetID(), source );
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
                score = ceil((double)cost/growth_rate/2.0) + days;
            }
            else {
                // For a neutral planet:
                //   the number of days to travel to the planet
                //   + time to regain units spent
                score = ceil((double)cost/growth_rate) + days;
            }
        }
        else {
            // hard case: we can arrive before some other ships
            // we know that this planet is (or will be) eventually be owned 
            // by an enemy
            cost = p.Cost(days);
            score = ceil((double)cost/growth_rate/2.0) + days; 
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
            LOG_FILE << "issueing order" << std::endl;
            pw.IssueOrder(Order( source, dest, num_ships ));
          }
    }
}

// This is a utility class that parses strings.
class StringUtil {
    public:
        // Tokenizes a string s into tokens. Tokens are delimited by any of the
        // characters in delimiters. Blank tokens are omitted.
        static void Tokenize(const std::string& s,
                const std::string& delimiters,
                std::vector<std::string>& tokens);

        // A more convenient way of calling the Tokenize() method.
        static std::vector<std::string> Tokenize(
                const std::string& s,
                const std::string& delimiters = std::string(" "));
};

void StringUtil::Tokenize(const std::string& s,
        const std::string& delimiters,
        std::vector<std::string>& tokens) {
    std::string::size_type lastPos = s.find_first_not_of(delimiters, 0);
    std::string::size_type pos = s.find_first_of(delimiters, lastPos);
    while (std::string::npos != pos || std::string::npos != lastPos) {
        tokens.push_back(s.substr(lastPos, pos - lastPos));
        lastPos = s.find_first_not_of(delimiters, pos);
        pos = s.find_first_of(delimiters, lastPos);
    }
}

std::vector<std::string> StringUtil::Tokenize(const std::string& s,
        const std::string& delimiters) {
    std::vector<std::string> tokens;
    Tokenize(s, delimiters, tokens);
    return tokens;
}

const int PLANET_X = 1;
const int PLANET_Y = 2;
const int PLANET_OWNER = 3;
const int PLANET_SHIPS = 4;
const int PLANET_GROWTH = 5;

const int FLEET_OWNER = 1;
const int FLEET_SHIPS = 2;
const int FLEET_SOURCE = 3;
const int FLEET_DEST = 4;
const int FLEET_LENGTH = 5;
const int FLEET_REMAINING = 6;

struct TempFleet {
    int owner;
    int dest;
    int ships;
    int remaining;
};

PlanetWars ParseGameState(const std::string& game_state) {
    std::vector<Planet> planets;
    std::vector<Fleet> fleets;
    std::vector<std::string> lines = StringUtil::Tokenize(game_state, "\n");
    int planet_id = 0;
    for (unsigned int i = 0; i < lines.size(); ++i) {
        std::string& line = lines[i];
        size_t comment_begin = line.find_first_of('#');
        if (comment_begin != std::string::npos) {
            line = line.substr(0, comment_begin);
        }
        std::vector<std::string> tokens = StringUtil::Tokenize(line);
        if (tokens.size() == 0) {
            continue;
        }
        if (tokens[0] == "P") {
            if (tokens.size() != 6) {
                throw "Invalid planet";
            }
            Planet p(
                    planet_id++,              // The ID of this planet
                    atoi(tokens[PLANET_OWNER].c_str()),  // Owner
                    atoi(tokens[PLANET_SHIPS].c_str())); // Num ships
            planets.push_back(p);
        } else if (tokens[0] == "F") {
            if (tokens.size() != 7) {
                throw "Invalid fleet";
            }
            // TempFleet f;
            // f.owner = atoi(tokens[FLEET_OWNER].c_str()); 
            // f.dest = atoi(tokens[FLEET_OWNER].c_str()); 
            // f.ships = atoi(tokens[FLEET_OWNER].c_str()); 
            // f.remaining = atoi(tokens[FLEET_OWNER].c_str()); 
            Fleet f(atoi(tokens[1].c_str()),  // Owner
                    atoi(tokens[2].c_str()),  // Num ships
                    atoi(tokens[3].c_str()),  // Source
                    atoi(tokens[4].c_str()),  // Destination
                    atoi(tokens[5].c_str()),  // Total trip length
                    atoi(tokens[6].c_str())); // Turns remaining
            fleets.push_back(f);
        } else {
            throw "Invalid object";
        }
    }

    // inform planets about fleets
    for (int i = 0; i < fleets.size(); ++i) {
        const Fleet& fleet = fleets[i];
        planets[ fleet.DestinationPlanet() ].AddIncomingFleet(fleet);
    }

    return PlanetWars(
        planets,
        fleets
    );
}

void ParseMap(const std::string& game_state) {
    std::vector<std::string> lines = StringUtil::Tokenize(game_state, "\n");
    for (unsigned int i = 0; i < lines.size(); ++i) {
        std::string& line = lines[i];
        size_t comment_begin = line.find_first_of('#');
        if (comment_begin != std::string::npos) {
            line = line.substr(0, comment_begin);
        }
        std::vector<std::string> tokens = StringUtil::Tokenize(line);
        if (tokens.size() == 0) {
            continue;
        }
        if (tokens[0] == "P") {
            if (tokens.size() != 6) {
                throw "Invalid planet";
            }
            Map::AddPlanet(
                atoi(tokens[PLANET_GROWTH].c_str()),
                atoi(tokens[PLANET_X].c_str()),
                atoi(tokens[PLANET_Y].c_str())
            );
        } else if (tokens[0] != "F") {
            throw "Invalid object";
        }
    }
}

void FinishTurn(const PlanetWars& pw) {
    std::vector<Order> orders = pw.Orders();

    // issue all the orders
    for ( int i=0; i < orders.size(); ++i ) {
        const Order &o = orders[i];

        if ( o.source == o.dest ) {
            // ensure we don't send ships to same planet
            continue;
        }
        else if ( o.ships <= 0 ) {
            // ensure that the number of ships is positive
            continue;
        }

        std::cout << o.source << " "
            << o.dest << " "
            << o.ships << std::endl;
        std::cout.flush();
    }

    // finish up
    std::cout << "go" << std::endl;
    std::cout.flush();
}

// This is just the main game loop that takes care of communicating with the
// game engine for you. You don't have to understand or change the code below.
int main(int argc, char *argv[]) {
    std::string current_line;
    std::string map_data;
    int turn_number = 0;

    LOG_FILE.open("debug.log");

    while (true) {
        int c = std::cin.get();
        current_line += (char)c;
        if (c == '\n') {
            if (current_line.length() >= 2 && current_line.substr(0, 2) == "go") {
                if ( turn_number == 0 ) {
                    // On the first turn we calculate the global map data
                    ParseMap(map_data);
                }

                ++turn_number;
                PlanetWars pw = ParseGameState(map_data);
                DoTurn(pw);
                FinishTurn(pw);
                map_data = "";
            } else {
                map_data += current_line;
            }
            current_line = "";
        }
    }

    LOG_FILE.close();

    return 0;
}
