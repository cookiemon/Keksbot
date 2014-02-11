#ifndef LOGGING_H
#define LOGGING_H

#ifdef _WIN32
#pragma error "Not implemented"
#else

#include <stdarg.h>
#include <syslog.h>

static inline void OpenLogging(const char* id)
{
	openlog(id, LOG_PERROR | LOG_PID, LOG_USER);
}

static inline void CloseLogging()
{
	closelog();
}

static inline void Log(int prio, const char* msg, ...)
{
	va_list va;
	va_start(va, msg);
	vsyslog(prio, msg, va);
	va_end(va);
}

#endif

#endif
