#ifndef LOG_H_
#define LOG_H_

#ifdef DEBUG

#include <fstream>
extern std::ofstream LOG_FILE;
extern std::ofstream LOG_ERROR_FILE;
#define LOG_(s)       LOG_FILE << s
#define LOG(s)        LOG_(s) << std::endl
#define LOG_ERROR(s)  LOG_ERROR_FILE << s << std::endl; LOG( "ERROR: " << s)
#define LOG_FLUSH()   LOG_FILE.flush(); LOG_ERROR_FILE.flush()
#define LOG_INIT(x,y) LOG_FILE.open(x); LOG_ERROR_FILE.open(y,std::fstream::app) 
#define LOG_CLOSE()   LOG_FILE.close(); LOG_ERROR_FILE.close()

#else // DEBUG

#define LOG_(s)
#define LOG(s)
#define LOG_ERROR(s)
#define LOG_FLUSH()
#define LOG_INIT(x,y)
#define LOG_CLOSE()

#endif // DEBUG

#endif // LOG_H

