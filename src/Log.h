#ifndef LOG_H_
#define LOG_H_

#ifdef DEBUG

#include <fstream>
#include <string>

extern std::ofstream LOG_FILE;
extern std::ofstream LOG_ERROR_FILE;
extern std::string PROG_NAME;

#define LOG_ERROR_FILENAME "error.log"

#define LOG_(s)         LOG_FILE << s
#define LOG(s)          LOG_(s) << std::endl
#define LOG_ERROR(s)    LOG_ERROR_FILE << PROG_NAME << ": " << s << std::endl; LOG( "ERROR: " << s)
#define LOG_FLUSH()     LOG_FILE.flush(); LOG_ERROR_FILE.flush()
#define LOG_CLOSE()     LOG_FILE.close(); LOG_ERROR_FILE.close()

inline void LOG_INIT(const char* prog_name) {
    // Just get the basename of the program
    PROG_NAME = std::string(prog_name);
    int pos = PROG_NAME.rfind('/');
    if ( pos != std::string::npos ) {
        PROG_NAME.replace(0, pos+1, "");
    }

    // open the log files
    std::string log_filename = PROG_NAME + ".log";
    LOG_FILE.open(log_filename.c_str());
    LOG_ERROR_FILE.open(LOG_ERROR_FILENAME,std::fstream::app);
}

#else // DEBUG

#define LOG_(s)
#define LOG(s)
#define LOG_ERROR(s)
#define LOG_FLUSH()
#define LOG_CLOSE()

inline void LOG_INIT(char* prog_name) {};

#endif // DEBUG

#endif // LOG_H
