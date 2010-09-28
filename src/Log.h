#ifndef LOG_H_
#define LOG_H_

#include <string>

#ifdef DEBUG
#include <fstream>

extern std::ofstream LOG_FILE;
extern std::ofstream LOG_ERROR_FILE;
extern std::string PROG_NAME;

#define LOG_ERROR_FILENAME "error.log"

#define LOG_(s)         LOG_FILE << s
#define LOG(s)          LOG_(s) << "\n"
#define LOG_ERROR(s)    { LOG_ERROR_FILE << PROG_NAME << ": " << s << std::endl; LOG( "ERROR: " << s); }
#define LOG_FLUSH()     { LOG_FILE.flush(); LOG_ERROR_FILE.flush(); }
#define LOG_CLOSE()     { LOG_FILE.close(); LOG_ERROR_FILE.close(); }

inline void LOG_INIT(const char* prog_name, std::string log_filename) {
    // determine the name of the program
    PROG_NAME = std::string(prog_name);
    int pos = PROG_NAME.rfind('/');
    if ( pos != std::string::npos ) {
        PROG_NAME.replace(0, pos+1, "");
    }

    // Use the program name for the log file if none is given
    if ( log_filename.empty() ) {
        log_filename = PROG_NAME + ".log";
    }

    // open the log files
    LOG_FILE.open(log_filename.c_str());
    LOG_ERROR_FILE.open(LOG_ERROR_FILENAME,std::fstream::app);
}

#else // DEBUG

#define LOG_(s)
#define LOG(s)
#define LOG_ERROR(s)
#define LOG_FLUSH()
#define LOG_CLOSE()

inline void LOG_INIT(char* prog_name, std::string log_filename) {};

#endif // DEBUG

#endif // LOG_H

