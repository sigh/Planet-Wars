#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <sys/time.h>
#include "PlanetWars.h"
#include "DoTurn.h"
#include "Log.h"

#ifdef DEBUG
std::ofstream LOG_FILE;
std::ofstream LOG_ERROR_FILE;
const char* PROG_NAME;
#endif

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

PlanetWars ParseGameState(const std::string& game_state) {
    std::vector<PlanetPtr> planets;
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
            PlanetPtr p( new Planet(
                planet_id++, // The ID of this planet
                atoi(tokens[PLANET_OWNER].c_str()),
                atoi(tokens[PLANET_SHIPS].c_str())
            ));
            planets.push_back(p);
        } else if (tokens[0] == "F") {
            if (tokens.size() != 7) {
                throw "Invalid fleet";
            }
            Fleet f;
            f.owner = atoi(tokens[FLEET_OWNER].c_str()); 
            f.dest = atoi(tokens[FLEET_DEST].c_str()); 
            f.ships = atoi(tokens[FLEET_SHIPS].c_str()); 
            f.remaining = atoi(tokens[FLEET_REMAINING].c_str()); 
            fleets.push_back(f);
        } else {
            throw "Invalid object";
        }
    }

    // inform planets about fleets
    for (int i = 0; i < fleets.size(); ++i) {
        const Fleet& f = fleets[i];
        planets[ f.dest ]->AddIncomingFleet(f);
    }

    return PlanetWars( planets );
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
                atof(tokens[PLANET_X].c_str()),
                atof(tokens[PLANET_Y].c_str())
            );
        } else if (tokens[0] != "F") {
            throw "Invalid object";
        }
    }

    Map::Init();
}

void FinishTurn(const PlanetWars& pw) {
    std::vector<Order> orders = pw.Orders();
    int num_ships = 0;

    // issue all the orders
    for ( int i=0; i < orders.size(); ++i ) {
        const Order &o = orders[i];

        // TODO: Check if ships the planet has enough ships
        if ( o.source == o.dest || o.ships <= 0 ) {
            // ensure that the number of ships is positive
            continue;
        }

        std::cout << o.source << " "
            << o.dest << " "
            << o.ships << std::endl;
        std::cout.flush();

        num_ships += o.ships;
    }

    // finish up
    std::cout << "go" << std::endl;
    std::cout.flush();

    // Log AFTER we have sent the command (wastes less time)
    LOG( orders.size() << " Orders (" << num_ships << " ships):" );

    for ( int i=0; i < orders.size(); ++i ) {
        const Order &o = orders[i];

        if ( o.source == o.dest || o.ships <= 0 ) {
            LOG_ERROR( "Invalid order: " << o.source << "->" << o.dest << " (" << o.ships << " ships)" );
        }
        else {
            LOG( o.source << "->" << o.dest << " (" << o.ships << " ships)" );
        }

    }
}

// This is just the main game loop that takes care of communicating with the
// game engine for you. You don't have to understand or change the code below.
int main(int argc, char *argv[]) {
    std::string current_line;
    std::string map_data;
    int turn_number = 0;

    LOG_INIT("debug_4.log", "error.log", argv[0]);
    LOG( "Start logging" );

    Config::Parse(argc, argv);

    while (true) {
        int c = std::cin.get();
        current_line += (char)c;
        if (c == '\n') {
            if (current_line.length() >= 2 && current_line.substr(0, 2) == "go") {
                timeval init;
                timeval finish;

                gettimeofday(&init,NULL);

                ++turn_number;

                if ( turn_number == 1 ) {
                    // On the first turn we calculate the global map data
                    ParseMap(map_data);
                }

                PlanetWars pw = ParseGameState(map_data);

                LOG( "== Turn " << turn_number << " ==" );

                LOG( "ME:    " << pw.Ships(ME) << "/" << pw.Production(ME) ); 
                LOG( "ENEMY: " << pw.Ships(ENEMY) << "/" << pw.Production(ENEMY) ); 

                // OMG how hacky... this is what passes for defence now
                // TODO: Remove this when we have DESTINATION BASED processing
                DoTurn(pw, turn_number);
                FinishTurn(pw);
                map_data = "";
                
                gettimeofday(&finish,NULL);
                LOG( "Time: " <<  ( finish.tv_usec - init.tv_usec ) );
                LOG_FLUSH();
            } else {
                map_data += current_line;
            }
            current_line = "";
        }
    }

    LOG_CLOSE();

    return 0;
}
