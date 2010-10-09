#include "Config.h"
#include "Log.h"
#include <string>
#include <sstream>
#include <map>
#include <iostream>
#include <vector>
#include <algorithm>

#ifdef DEBUG // for local (with boost)
#include <fstream>
#include <boost/program_options.hpp>
namespace po = boost::program_options;
#endif // DEBUG

class ConfigMapBase;
typedef std::pair<std::string,ConfigMapBase*> ConfigItem;

// This class allows us to store all config containers in the same place
class ConfigMapBase {
    public:
        virtual std::string String(const std::string& key) = 0;
        virtual void PopulateItems(std::vector<ConfigItem>& config_items) = 0;
#ifdef DEBUG
        virtual void SetupOptions(po::options_description& options) = 0;
#endif // DEBUG
};

template<typename T> class ConfigMap : public ConfigMapBase {
    public:
        typedef typename std::map<std::string,T>::iterator iterator;

        T& operator[] ( const std::string& key ) {
            return config_[key];
        }

        // return the vaue for the config item key
        //  or else throw an error
        T Value( const std::string& key ) {
            iterator found = config_.find(key);
            if ( found != config_.end() ) {
                return found->second;
            }

            // program should never be asking for keys that don't exist
            std::string error = "Invalid config item: " + key;
            LOG_ERROR(error);
            throw error;
        }

        // return the value for the given key as a string
        std::string String( const std::string& key ) {
            std::stringstream s;
            s << Value(key);
            return s.str();
        }

        void PopulateItems(std::vector<ConfigItem>& config_items) {
            iterator it;
            for ( it=config_.begin(); it != config_.end(); ++it ) {
                config_items.push_back( ConfigItem(it->first, this) );
            }
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
    ConfigMapBase* config_maps[] = {
        &int_config_, &bool_config_, &double_config_, &string_config_, NULL    
    };

    // Lookup values of each type
    template<> int    Value<int>(const std::string& key) { return int_config_.Value(key); }
    template<> bool   Value<bool>(const std::string& key) { return bool_config_.Value(key); }
    template<> double Value<double>(const std::string& key) { return double_config_.Value(key); }
    template<> std::string Value<std::string>(const std::string& key) { return string_config_.Value(key); }

    void Parse(int argc, char*argv[]);

    // setup config default
    void SetupDefaults() {
        bool_config_["defence"] = 1;
        int_config_["antirage"] = 2;
        bool_config_["antirage.exlusions"] = 1;
        bool_config_["attack"] = 1;
        int_config_["attack.max_delay"] = 0;
        bool_config_["redist"] = 1;
        bool_config_["redist.future"] = 0;
        int_config_["redist.slack"] = 0;
        bool_config_["flee"] = 0;

        double_config_["cost.distance_scale"] = 2.0;
        double_config_["cost.growth_scale"] = 2.0;
        int_config_["cost.offset"] = 3;
        double_config_["cost.delay_scale"] = 2.0;
        bool_config_["cost.use_egr"] = 0;

        string_config_["config_file"] = "MyBot.conf";
        string_config_["log_file"] = "";

        // The following is just so the compile details will be logged
        string_config_["_compile.time"] = __DATE__ " " __TIME__;

        // so much trouble just to get the git revision in quotes o_O
#ifdef GIT_REVISION
#define STRINGIFY(a) #a
#define STRINGIFY_MACRO(a) STRINGIFY(a)
        string_config_["_compile.revision"] = STRINGIFY_MACRO(GIT_REVISION);
#endif
    }

    // init config
    void Init(int argc, char *argv[]) {
        SetupDefaults();
        Parse(argc, argv);
    }

    // Convert the options to a string
    std::string String() {
        std::vector<ConfigItem> config_items;

        // add all config items to a vector
        for ( ConfigMapBase** current_map = config_maps; *current_map != NULL; ++current_map ) {
            (*current_map)->PopulateItems(config_items);
        }

        // sort the vector
        sort(config_items.begin(), config_items.end());

        // output the config items in sorted order
        std::vector<ConfigItem>::iterator it;
        std::stringstream s;
        for ( it=config_items.begin(); it != config_items.end(); ++it ) {
            s << it->first << "=" << it->second->String(it->first) << "\n";
        }
        return s.str();
    }

#ifdef DEBUG // for local (with boost)

    void Parse(int argc, char*argv[]) {
        po::options_description options("Options");

        for ( ConfigMapBase** current_map = config_maps; *current_map != NULL; ++current_map ) {
            (*current_map)->SetupOptions(options);
        }

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
