#ifndef LOGGER_H
#define LOGGER_H

#include <semaphore.h>
#include <iostream>

enum LOG_LEVEL { NO_INFO=0, ERROR=1, NOT_IMPLEMENTED=2,CALLS=3,TRACE=4};

#if 1
#define LOG(level,esp)					\
{							\
	if(level<=Log::getLevel())			\
	{						\
		int __a__;						\
		pthread_testcancel();				\
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,&__a__); \
		Log l(level);				\
		l() << esp << std::endl;		\
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&__a__); \
	}						\
}
#else
#define LOG(level,esp) {}
#endif

class Log
{
private:
	static sem_t mutex;
	LOG_LEVEL cur_level;
	bool valid;
	static const char* level_names[];
	static LOG_LEVEL log_level;

public:
	Log(LOG_LEVEL l);
	~Log();
	std::ostream& operator()();
	operator bool() { return valid; }
	static void initLogging(LOG_LEVEL l);
	static LOG_LEVEL getLevel(){return log_level;}

};

#endif
