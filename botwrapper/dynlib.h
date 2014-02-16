#ifndef DYNLIB_H
#define DYNLIB_H

#ifdef _WIN32
#pragma error "Not implemented"
#else
#include <dlfcn.h>
#include "logging.h"
#include <stdlib.h>
typedef void* DynLibHandle;

DynLibHandle OpenDynLib(const char* libName)
{
	void* dlHandle = dlopen(libName, RTLD_NOLOAD | RTLD_LAZY | RTLD_LOCAL);
	if(dlHandle != NULL)
	{
		Log(LOG_ERR, "Library %s was already loaded", libName);
		return dlHandle;
	}
	return dlopen(libName, RTLD_LAZY | RTLD_LOCAL);
}

int CloseDynLib(DynLibHandle lib)
{
	return dlclose(lib);
}

void* GetDynLibProc(DynLibHandle lib, const char* procName)
{
	return dlsym(lib, procName);
}

char* LastDynLibError(void)
{
	return dlerror();
}
#endif

#endif
