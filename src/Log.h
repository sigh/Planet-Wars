#ifndef LOG_H_
#define LOG_H_

#ifdef DEBUG

#include <fstream>
extern std::ofstream LOG_FILE;
extern std::ofstream LOG_ERROR_FILE;
extern const char* PROG_NAME;
#define LOG_(s)         LOG_FILE << s
#define LOG(s)          LOG_(s) << std::endl
#define LOG_ERROR(s)    LOG_ERROR_FILE << PROG_NAME << ": " << s << std::endl; LOG( "ERROR: " << s)
#define LOG_FLUSH()     LOG_FILE.flush(); LOG_ERROR_FILE.flush()
#define LOG_CLOSE()     LOG_FILE.close(); LOG_ERROR_FILE.close()

inline void LOG_INIT(const char* log_filename, const char* log_error_filename, const char* prog_name) {
    LOG_FILE.open(log_filename);
    LOG_ERROR_FILE.open(log_error_filename,std::fstream::app);
    PROG_NAME = prog_name;
}

#else // DEBUG

#define LOG_(s)
#define LOG(s)
#define LOG_ERROR(s)
#define LOG_FLUSH()
#define LOG_CLOSE()

inline void LOG_INIT(char* log_filename, char* log_error_filename, char* prog_name);

#endif // DEBUG

#endif // LOG_H

