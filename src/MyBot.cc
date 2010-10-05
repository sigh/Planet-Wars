#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <sys/time.h>
#include "GameState.h"
#include "Config.h"
#include "DoTurn.h"
#include "Log.h"

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#ifdef DEBUG
std::ofstream LOG_FILE;
std::ofstream LOG_ERROR_FILE;
std::string PROG_NAME;
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

// convert player id from external representation to internal representation
int ConvertPlayerID(int player_id) {
    if ( player_id == 1 ) {
        return ME;
    }
    else if ( player_id == 2 ) {
        return ENEMY;
    }
    else {
        return NEUTRAL;
    }
}

GameState ParseGameState(const std::string& game_state) {
    static int turn = 0;
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
                ConvertPlayerID(atoi(tokens[PLANET_OWNER].c_str())),
                atoi(tokens[PLANET_SHIPS].c_str())
            ));
            planets.push_back(p);
        } else if (tokens[0] == "F") {
            if (tokens.size() != 7) {
                throw "Invalid fleet";
            }
            fleets.push_back(Fleet(
                ConvertPlayerID(atoi(tokens[FLEET_OWNER].c_str())), 
                atoi(tokens[FLEET_SOURCE].c_str()),
                atoi(tokens[FLEET_DEST].c_str()),
                atoi(tokens[FLEET_SHIPS].c_str()),
                atoi(tokens[FLEET_REMAINING].c_str()) - atoi(tokens[FLEET_LENGTH].c_str()) 
            ));
        } else {
            throw "Invalid object";
        }
    }

    turn += 1;

    // TODO: make this less coypying around
    GameState state(turn, planets);

    // inform planets about fleets
    foreach ( const Fleet& f, fleets ) {
        state.AddFleet(f);
    }

    return state;
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

void FinishTurn(const GameState& state, const std::vector<Fleet>& orders) {
    int num_ships = 0;

    // issue all the orders
    foreach ( const Fleet& o, orders ) {
        if ( o.source == o.dest 
                || o.ships <= 0 
                || o.ships > state.Planet(o.source)->Ships() 
                || state.Planet(o.source)->Owner() != ME 
        ) {
            // ensure the order is not to the same planet
            // and that the number of ships is positive
            // and that the source has enough ships
            // and that we own the planet
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

    foreach ( const Fleet& o, orders ) {
        // TODO: seperate out and have different log message for each condition
        if ( o.source == o.dest 
                || o.ships <= 0 
                || o.ships > state.Planet(o.source)->Ships() 
                || state.Planet(o.source)->Owner() != ME 
        ) {
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
    bool map_parsed = false;

    Config::Init(argc, argv);

    LOG_INIT(argv[0], Config::Value<std::string>("log_file"));

    // log the command used to run this program
    for ( int i=0; i < argc; ++i ) { LOG_( argv[i] << " " ); } LOG(""); LOG("");

    // log the config options
    LOG("== OPTIONS ==");
    LOG_(Config::String());

    while (true) {
        int c = std::cin.get();
        current_line += (char)c;
        if (c == '\n') {
            if (current_line.length() >= 2 && current_line.substr(0, 2) == "go") {
                timeval init;
                timeval finish;

                gettimeofday(&init,NULL);

                if ( ! map_parsed ) {
                    // On the first turn we calculate the global map data
                    ParseMap(map_data);
                    map_parsed = true;
                }

                const GameState state = ParseGameState(map_data);

                LOG("");
                LOG( "== Turn " << state.Turn() << " ==" );

                LOG( "ME:    " << state.Ships(ME) << "/" << state.Production(ME) ); 
                LOG( "ENEMY: " << state.Ships(ENEMY) << "/" << state.Production(ENEMY) ); 

                std::vector<Fleet> orders;
                DoTurn(state, orders);
                FinishTurn(state, orders);
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
