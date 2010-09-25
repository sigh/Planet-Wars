#include "Config.h"
#include "Log.h"
#include <string>
#include <map>
#include <iostream>

#ifdef DEBUG // for local (with boost)
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
        void Print() {
            iterator it;
            for ( it = config_.begin(); it != config_.end(); ++it ) {
                std::cout << it->first << " = " << it->second << std::endl;
            }
        }

    private: 
        std::map<std::string,T> config_;
};

namespace Config {
    ConfigMap<int> int_config_;
    ConfigMap<bool> bool_config_;
    ConfigMap<double> double_config_;

    void Parse(int argc, char*argv[]);

    // setup config default
    void SetupDefaults() {
        int_config_["foo"] = 1;
        bool_config_["bar"] = false;
        double_config_["baz"] = 1.2;
    }

    // init config
    void Init(int argc, char *argv[]) {
        SetupDefaults();
        Parse(argc, argv);
    }

    // Lookup values of each type
    template<> int    Value<int>   (const std::string& key) { return int_config_.find(key); }
    template<> bool   Value<bool>  (const std::string& key) { return bool_config_.find(key); }
    template<> double Value<double>(const std::string& key) { return double_config_.find(key); }

    // print all config options
    void Print() {
        double_config_.Print();
        int_config_.Print();
        bool_config_.Print();
    }

#ifdef DEBUG // for local (with boost)
    void Parse(int argc, char*argv[]) {

    }
#else // for the main contest - no boost 
    // no command line options in the main contest
    void Parse(int argc, char*argv[]) { }
#endif // DEBUG
}
