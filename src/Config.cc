#include "Config.h"
#include <string>
#include <map>
#include <iostream>

#ifdef DEBUG // for local (with boost)
#include <boost/program_options.hpp>
namespace po = boost::program_options;
#endif // DEBUG

template<typename T> class ConfigMap {
    public:
        T& operator[] ( const std::string& key ) {
            return config_[key];
        }

        void Print() {
            std::cout << "foo";
            // std::map<std::string,T>::iterator it;
            // for ( it = config_.begin(); it != config_.end(); ++it ) {
            //     cout << it->first << " = " << it->second;
            // }
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
