#ifndef CONFIG_H_
#define CONFIG_H_

#include <string>

namespace Config {
    void Init(int argc, char*argv[]);
    void Print();

    template<typename T> T Value( const std::string& key );
}

#endif
