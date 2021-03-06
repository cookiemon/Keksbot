#ifndef LOGGING_H
#define LOGGING_H

#ifdef _WIN32
#pragma error "Not implemented"
#else

#include <stdarg.h>
#include <syslog.h>

static void Log(int prio, const char* msg, ...)
{
	va_list va;
	va_start(va, msg);
	vsyslog(prio, msg, va);
	va_end(va);
}

#endif

#endif
