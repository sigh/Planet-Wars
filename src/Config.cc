#include "Config.h"
#include "Log.h"
#include <string>
#include <sstream>
#include <map>
#include <iostream>

#ifdef DEBUG // for local (with boost)
#include <fstream>
#include <boost/program_options.hpp>
namespace po = boost::program_options;
#endif // DEBUG

template<typename T> class ConfigMap {
    public:
        typedef typename std::map<std::string,T>::iterator iterator;

        T& operator[] ( const std::string& key ) {
            return config_[key];
        }

        // return the vaue for the config item key
        //  or else throw an error
        T find( const std::string& key ) {
            iterator found = config_.find(key);
            if ( found != config_.end() ) {
                return found->second;
            }

            // program should never be asking for keys that don't exist
            std::string error = "Invalid config item: " + key;
            LOG_ERROR(error);
            throw error;
        }

        // Print out all the config items
        std::string String() {
            std::stringstream s;
            iterator it;
            for ( it = config_.begin(); it != config_.end(); ++it ) {
                s << it->first << "=" << it->second << std::endl;
            }
            return s.str();
        }

        iterator begin() { return config_.begin(); }

        iterator end() { return config_.end(); }

#ifdef DEBUG
        void SetupOptions(po::options_description& options) {
            iterator it;
            for ( it = config_.begin(); it != config_.end(); ++it ) {
                options.add_options()(it->first.c_str(), po::value<T>(&(it->second))->default_value(it->second));
            }
        }
#endif // DEBUG

    private: 
        std::map<std::string,T> config_;
};

namespace Config {
    ConfigMap<int> int_config_;
    ConfigMap<bool> bool_config_;
    ConfigMap<double> double_config_;
    ConfigMap<std::string> string_config_;

    void Parse(int argc, char*argv[]);

    // setup config default
    void SetupDefaults() {
        bool_config_["defence"] = 1;
        int_config_["antirage"] = 1;
        bool_config_["antirage.exlusions"] = 1;
        bool_config_["attack"] = 1;
        bool_config_["redist"] = 1;
        bool_config_["flee"] = 0;

        double_config_["cost.distance_scale"] = 2.0;
        double_config_["cost.growth_scale"] = 2.0;
        int_config_["cost.offset"] = 3;

        string_config_["config_file"] = "MyBot.conf";
        string_config_["log_file"] = "";
    }

    // init config
    void Init(int argc, char *argv[]) {
        SetupDefaults();
        Parse(argc, argv);
    }

    // Lookup values of each type
    template<> int    Value<int>(const std::string& key) { return int_config_.find(key); }
    template<> bool   Value<bool>(const std::string& key) { return bool_config_.find(key); }
    template<> double Value<double>(const std::string& key) { return double_config_.find(key); }
    template<> std::string Value<std::string>(const std::string& key) { return string_config_.find(key); }

    // Convert the options to a string
    std::string String() {
        std::stringstream s;
        s   << double_config_.String()
            << int_config_.String()
            << bool_config_.String()
            << string_config_.String();
        return s.str();
    }

#ifdef DEBUG // for local (with boost)

    void Parse(int argc, char*argv[]) {
        po::options_description options("Options");

        int_config_.SetupOptions(options);
        bool_config_.SetupOptions(options);
        double_config_.SetupOptions(options);
        string_config_.SetupOptions(options);

        // command line options
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, options), vm);
        po::notify(vm);

        // config file options
        std::ifstream config_file(Value<std::string>("config_file").c_str());
        po::store(po::parse_config_file(config_file, options), vm);
        config_file.close();
        po::notify(vm);
    }
#else // for the main contest - no boost 
    // no command line options in the main contest
    void Parse(int argc, char*argv[]) { }
#endif // DEBUG
}
