#ifndef LOG_H_
#define LOG_H_

#ifdef DEBUG

#include <fstream>
extern std::ofstream LOG_FILE;
#define LOG_(x)      LOG_FILE << x
#define LOG(x)       LOG_(x) << std::endl
#define LOG_FLUSH()  LOG_FILE.flush()
#define LOG_INIT(x)  LOG_FILE.open(x)
#define LOG_CLOSE(x) LOG_FILE.close()

#else // DEBUG


#define LOG_(x)
#define LOG(x)
#define LOG_FLUSH()
#define LOG_INIT(x)
#define LOG_CLOSE()

#endif // DEBUG

#endif // LOG_H

